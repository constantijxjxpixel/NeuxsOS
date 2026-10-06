#include "shell.h"
#include "keyboard.h"
#include "vga.h"
#include "serial.h"
#include "timer.h"
#include "rtc.h"
#include "pmm.h"
#include "kmalloc.h"
#include "fs.h"
#include "mouse.h"
#include "speaker.h"
#include "editor.h"
#include "task.h"
#include "elf.h"
#include "net.h"
#include <stdint.h>

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

static char *out_buf = 0;
static int out_pos = 0, out_max = 0;
static const char *in_buf = 0;
static int in_len = 0;

static void kputc(char c){
    if(out_buf){ if(out_pos<out_max-1) out_buf[out_pos++]=c; return; }
    char b[2]={c,0}; vga_puts(b); serial_puts(b);
}
static void kputs(const char *s){ while(*s) kputc(*s++); }

static int kstrcmp(const char *a,const char *b){ while(*a&&*b&&*a==*b){a++;b++;} return *a-*b; }
static int kstrncmp(const char *a,const char *b,int n){ for(int i=0;i<n;i++){ if(a[i]!=b[i])return a[i]-b[i]; if(!a[i])return 0; } return 0; }
static int kstrlen(const char *s){ int l=0; while(s[l])l++; return l; }
static int katoi(const char *s){ int n=0,neg=0; while(*s==' ')s++; if(*s=='-'){neg=1;s++;} while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;} return neg?-n:n; }
static void print_num(uint32_t n){ char b[12]; int i=11; b[i]=0; if(!n){kputc('0');return;} while(n){b[--i]=(char)('0'+(n%10));n/=10;} kputs(b+i); }
static void print2(uint8_t v){ if(v<10)kputc('0'); print_num(v); }
static void print_hex(uint32_t v){ const char *d="0123456789ABCDEF"; char b[11]="0x00000000"; for(int i=9;i>=2;i--){b[i]=d[v&0xF];v>>=4;} kputs(b); }

static int tz_offset = 0;
static char hist[8][64];
static int hist_count = 0, hist_pos = -1;
static void hist_push(const char *l){ if(!l[0])return; for(int i=7;i>0;i--){for(int k=0;k<64;k++)hist[i][k]=hist[i-1][k];} for(int k=0;k<63;k++){hist[0][k]=l[k];if(!l[k])break;} hist[0][63]=0; if(hist_count<8)hist_count++; hist_pos=-1; }
static void hist_save(void){ char b[512]; int off=0; b[off++]=72;b[off++]=73;b[off++]=83;b[off++]=84; b[off++]=(char)hist_count; for(int i=0;i<hist_count;i++){ int L=0; while(hist[i][L]&&L<60)L++; b[off++]=(char)L; for(int k=0;k<L;k++)b[off++]=hist[i][k]; if(off>500)break; } for(int k=off;k<512;k++)b[k]=0; fs_write(".history",b,512); }
static void hist_load(void){ char b[512]; int r=fs_read(".history",b,512); if(r<512)return; if(b[0]!=72||b[1]!=73||b[2]!=83||b[3]!=84)return; hist_count=(unsigned char)b[4]; if(hist_count>8)hist_count=8; int off=5; for(int i=0;i<hist_count;i++){ int L=(unsigned char)b[off++]; if(L>60||off+L>512){hist_count=i;break;} for(int k=0;k<L;k++)hist[i][k]=b[off++]; hist[i][L]=0; } hist_pos=-1; }

static int bU(char c){ return (c>='a'&&c<='z')?(c-32):c; }
static int bAlpha(char c){ return (c>='a'&&c<='z')||(c>='A'&&c<='Z'); }
static void bputc(char c){ char b[2]={c,0}; vga_puts(b); serial_puts(b); }
static void bputs(const char *s){ while(*s) bputc(*s++); }
static void bnum(int n){ char b[12]; int i=11; b[i]=0; if(n==0){bputc('0');return;} int neg=n<0; if(neg)n=-n; while(n){b[--i]=(char)('0'+(n%10));n/=10;} if(neg)b[--i]='-'; bputs(b+i); }

#define MAXLINES 48
static int ln_num[MAXLINES];
static char ln_txt[MAXLINES][80];
static int nlines=0, vars[26];
static int end_flag=0, goto_target=-1, list_all=0;
static int pe_expr(const char **pp);
static int pe_factor(const char **pp){ const char *p=*pp; while(*p==' ')p++; if(*p=='('){p++;int v=pe_expr(&p);while(*p==' ')p++;if(*p==')')p++;*pp=p;return v;} if(*p>='0'&&*p<='9'){int v=0;while(*p>='0'&&*p<='9'){v=v*10+(*p-'0');p++;}*pp=p;return v;} if(bAlpha(*p)){int v=vars[(*p>='a')?(*p-'a'):(*p-'A')];p++;*pp=p;return v;} *pp=p;return 0; }
static int pe_term(const char **pp){ int v=pe_factor(pp); const char *p=*pp; for(;;){ while(*p==' ')p++; if(*p=='*'){p++;v=v*pe_factor(&p);} else if(*p=='/'){p++;int d=pe_factor(&p);v=d?v/d:0;} else break; *pp=p; } *pp=p;return v; }
static int pe_expr_(const char **pp){ int v=pe_term(pp); const char *p=*pp; for(;;){ while(*p==' ')p++; if(*p=='+'){p++;v+=pe_term(&p);} else if(*p=='-'){p++;v-=pe_term(&p);} else break; *pp=p; } *pp=p;return v; }
static int pe_expr(const char **pp){ return pe_expr_(pp); }
static int pe_cond(const char **pp){ int a=pe_expr(pp); const char *p=*pp; while(*p==' ')p++; int op=-1; if(p[0]=='<'&&p[1]=='='){op=0;p+=2;} else if(p[0]=='>'&&p[1]=='='){op=1;p+=2;} else if(p[0]=='<'&&p[1]=='>'){op=2;p+=2;} else if(p[0]=='='){op=3;p++;} else if(p[0]=='<'){op=4;p++;} else if(p[0]=='>'){op=5;p++;} else {*pp=p;return a;} int b=pe_expr(&p); *pp=p; switch(op){case 0:return a<=b;case 1:return a>=b;case 2:return a!=b;case 3:return a==b;case 4:return a<b;case 5:return a>b;} return 0; }
static int find_line(int num){ for(int i=0;i<nlines;i++) if(ln_num[i]==num) return i; return -1; }
static void run_one(const char *s){
    while(*s==' ')s++; if(!*s)return;
    if(bU(s[0])=='R'&&bU(s[1])=='E') return;
    if(bU(s[0])=='E'&&bU(s[1])=='N'&&bU(s[2])=='D'){ end_flag=1; return; }
    if(bU(s[0])=='G'&&bU(s[1])=='O'&&bU(s[2])=='T'&&bU(s[3])=='O'){ const char *q=s+4; while(*q==' ')q++; int t=0; while(*q>='0'&&*q<='9'){t=t*10+(*q-'0');q++;} goto_target=t; return; }
    if(bU(s[0])=='L'&&bU(s[1])=='I'&&bU(s[2])=='S'&&bU(s[3])=='T'){ list_all=1; return; }
    const char *p=s;
    if(bU(p[0])=='L'&&bU(p[1])=='E'&&bU(p[2])=='T'){ p+=3; while(*p==' ')p++; }
    if(bAlpha(p[0])){ const char *eq=p; while(*eq&&*eq!='=')eq++; if(*eq=='='){ int idx=(p[0]>='a')?(p[0]-'a'):(p[0]-'A'); const char *vp=eq+1; vars[idx]=pe_expr(&vp); return; } }
    if(bU(s[0])=='P'&&bU(s[1])=='R'&&bU(s[2])=='I'&&bU(s[3])=='N'&&bU(s[4])=='T'){ const char *q=s+5; while(*q==' ')q++; if(*q=='"'){q++;while(*q&&*q!='"'){bputc(*q);q++;} bputc(10);return;} bnum(pe_expr(&q)); bputc(10); return; }
    bputs("?SYNTAX: "); bputs(s); bputc(10);
}
static void exec_stmt(const char *s){
    while(*s==' ')s++;
    if(bU(s[0])=='I'&&bU(s[1])=='F'){ const char *q=s+2; int c=pe_cond(&q); while(*q==' ')q++; if(bU(q[0])=='T'&&bU(q[1])=='H'&&bU(q[2])=='E'&&bU(q[3])=='N')q+=4; while(*q==' ')q++; const char *then=q,*els=0; for(const char *r=q;*r;r++){ if(bU(r[0])=='E'&&bU(r[1])=='L'&&bU(r[2])=='S'&&bU(r[3])=='E'){els=r;break;} } if(els){ char part[80]; int k=0; while(q+k<els&&k<79){part[k]=q[k];k++;} part[k]=0; if(c)run_one(part); else run_one(els+4); } else { if(c)run_one(then); } return; }
    run_one(s);
}
static void run_program(void){ int pc=0,steps=0; end_flag=0; goto_target=-1; list_all=0; while(pc>=0&&pc<nlines&&steps<50000&&!end_flag){ steps++; exec_stmt(ln_txt[pc]); if(end_flag)break; if(goto_target>=0){pc=find_line(goto_target);goto_target=-1;continue;} if(list_all){ for(int i=0;i<nlines;i++){bnum(ln_num[i]);bputc(' ');bputs(ln_txt[i]);bputc(10);} list_all=0; } pc++; } }
static const char *DEMO =
"10 REM NexusBASIC demo\n"
"20 PRINT \"Hello from NexusBASIC\"\n"
"30 LET a=2+3\n"
"40 PRINT a\n"
"50 IF a>4 THEN PRINT \"big\" ELSE PRINT \"small\"\n"
"60 LET i=1\n"
"70 PRINT i\n"
"80 LET i=i+1\n"
"90 IF i<=5 THEN GOTO 70\n"
"100 END\n";
static void load_text(const char *src){ nlines=0; int autonum=10; const char *p=src; while(*p&&nlines<MAXLINES){ const char *e=p; while(*e&&*e!='\n')e++; int len=(int)(e-p); if(len>79)len=79; char line[80]; for(int k=0;k<len;k++)line[k]=p[k]; line[len]=0; const char *t=line; while(*t==' ')t++; int num=0; const char *q=t; if(*q>='0'&&*q<='9'){ while(*q>='0'&&*q<='9'){num=num*10+(*q-'0');q++;} while(*q==' ')q++; t=q; } else { num=autonum; autonum+=10; } ln_num[nlines]=num; int k=0; while(t[k]&&t[k]!='\r'&&k<79){ln_txt[nlines][k]=t[k];k++;} ln_txt[nlines][k]=0; nlines++; p=(*e)?e+1:e; } }
static void basic_run(const char *arg){ static char buf[2048]; const char *src=DEMO; if(arg&&arg[0]){ int r=fs_read(arg,buf,2047); if(r<0){ bputs("basic: no such file: "); bputs(arg); bputc(10); return; } buf[r]=0; src=buf; } for(int i=0;i<26;i++)vars[i]=0; load_text(src); bputs("--- NexusBASIC 1.0 ---\n"); run_program(); }

static void cmd_reboot(void){ kputs("Rebooting...\n"); uint8_t g=0x02; while(g&0x02) g=inb(0x64); outb(0x64,0xFE); for(;;); }
static void cmd_help(void){ kputs("Commands:\n  help ver date uptime sleep N clear echo reboot\n  mem ptest malloc N ps spawn X kill N panic\n  ls [-l] cat F touch F rm F write F T edit F sync\n  mkdir D cd D pwd beep [f] mouse tz N run S\n  net ping A.B.C.D wc grep P basic [file.bas]\n  cmd > file, cmd < file, cmd1 | cmd2\n  Up/Down = history\n"); }
static void cmd_ver(void){ kputs("NexusOS 1.0 (Stage 11: FAT fs + NexusBASIC)\nCode & Productivity in One Core\n"); }
static void cmd_date(void){ struct rtc_time t; rtc_read(&t); int h=(t.hour+tz_offset)%24; if(h<0)h+=24; print_num(t.year);kputc('-');print2(t.month);kputc('-');print2(t.day);kputc(' ');print2((uint8_t)h);kputc(':');print2(t.minute);kputc(':');print2(t.second);kputs(" UTC"); if(tz_offset>=0)kputc('+'); print_num((uint32_t)(tz_offset<0?-tz_offset:tz_offset)); kputc('\n'); }
static void cmd_tz(const char *a){ tz_offset=katoi(a); kputs("timezone offset set\n"); }
static void cmd_uptime(void){ kputs("Uptime: "); print_num(timer_get_seconds()); kputs(" s ("); print_num(timer_get_ticks()); kputs(" ticks)\n"); }
static void cmd_sleep(const char *a){ int n=katoi(a); if(n<=0){kputs("Usage: sleep <sec>\n");return;} task_sleep((uint32_t)n); kputs("Done.\n"); }
static void cmd_mem(void){ kputs("RAM: "); print_num(pmm_total_pages()*4); kputs(" KB | free pages: "); print_num(pmm_free_pages()); kputs(" | heap: "); print_num(kheap_used()); kputs("/131072 B\n"); }
static void cmd_ptest(void){ uint32_t p=pmm_alloc_page(); if(!p){kputs("no free pages\n");return;} volatile uint8_t *m=(volatile uint8_t*)p; for(int i=0;i<4096;i++)m[i]=(uint8_t)i; int ok=1; for(int i=0;i<4096;i++) if(m[i]!=(uint8_t)i){ok=0;break;} kputs(ok?"page test OK at ":"page test FAIL at "); print_hex(p); kputc('\n'); pmm_free_page(p); }
static void cmd_malloc(const char *a){ int n=katoi(a); if(n<=0){kputs("Usage: malloc <bytes>\n");return;} void *p=kmalloc((uint32_t)n); if(!p){kputs("heap exhausted\n");return;} kputs("kmalloc="); print_hex((uint32_t)p); kputc('\n'); }
static void cmd_ls(const char *a){ int show=(a[0]==45&&a[1]==108); int cur=fs_get_cwd(); kputs("Directory contents:\n"); for(int i=0;i<FS_MAX_NODES;i++){ if(!nodes[i].used||nodes[i].parent!=cur)continue; kputs("  "); if(show)kputs(nodes[i].type==FS_DIR?"D ":"F "); kputs(nodes[i].name); if(show){kputs("  ");print_num((uint32_t)nodes[i].data_len);kputs(" B");} kputc(10);} }
static void cmd_mkdir(const char *a){ if(fs_mkdir(a)<0)kputs("mkdir: failed\n"); }
static void cmd_cd(const char *a){ int idx=fs_resolve(a); if(idx<0||nodes[idx].type!=FS_DIR){kputs("cd: not a directory\n");return;} fs_set_cwd(idx); }
static void cmd_pwd(void){ int cur=fs_get_cwd(); if(cur==0){kputs("/\n");return;} char path[256]; int pos=0,stack[16],top=0; while(cur!=0){stack[top++]=cur;cur=nodes[cur].parent;} for(int i=top-1;i>=0;i--){path[pos++]=47;const char *n=nodes[stack[i]].name;while(*n){path[pos++]=*n++;}} if(pos==0)path[pos++]=47; path[pos]=0; kputs(path); kputc(10); }
static void cmd_cat(const char *a){ char b[512]; int r=fs_read(a,b,511); if(r<0){kputs("cat: not found: ");kputs(a);kputc('\n');return;} b[r]=0; kputs(b); }
static void cmd_touch(const char *a){ if(fs_touch(a)<0)kputs("touch: fs full\n"); }
static void cmd_rm(const char *a){ if(fs_rm(a)<0)kputs("rm: not found\n"); }
static void cmd_write(const char *a){ const char *sp=a; while(*sp&&*sp!=' ')sp++; if(!*sp){kputs("Usage: write <file> <text>\n");return;} char nm[24]; int i=0; while(a+i<sp&&i<23){nm[i]=a[i];i++;} nm[i]=0; fs_write(nm,sp+1,-1); kputs("written: ");kputs(nm);kputc('\n'); }
static void cmd_sync(void){ fs_sync(); kputs("filesystem synced to disk\n"); }
static void cmd_beep(const char *a){ int f=katoi(a); if(f<50)f=440; speaker_beep((uint32_t)f,200); }
static void cmd_panic(void){ __asm__ volatile("int $0"); }
static void cmd_mouse(void){ vga_clear(); kputs("Mouse demo. ESC exits.\n"); int ox=-1,oy=-1; uint16_t saved=0; for(;;){ if(keyboard_scan_pending()){uint8_t s=keyboard_getscan(); if(s==0x01){keyboard_flush();break;}} int x,y,b; mouse_get(&x,&y,&b); if(x!=ox||y!=oy){ if(ox>=0)vga_put_at(ox,oy,saved); saved=vga_get_at(x,y); vga_putc_at(x,y,(char)0xDB,VGA_WHITE,VGA_LRED); ox=x;oy=y; } if(b&1)vga_putc_at(x,y,'*',VGA_LGREEN,VGA_BLACK); __asm__ volatile("hlt"); } vga_clear(); }
static const char *state_str(uint32_t s){ if(s==TASK_READY)return "READY "; if(s==TASK_RUNNING)return "RUN   "; if(s==TASK_SLEEPING)return "SLEEP "; if(s==TASK_DEAD)return "DEAD  "; return "FREE  "; }
static void cmd_ps(void){ kputs("PID  STATE   NAME\n"); for(int i=0;i<TASK_MAX;i++){ if(!tasks[i].used)continue; print_num(tasks[i].pid);kputs("    ");kputs(state_str(tasks[i].state));kputs("  ");kputs(tasks[i].name);kputc('\n'); } }
static void cmd_spawn(const char *a){ int p=-1; if(kstrcmp(a,"clock")==0)p=task_spawn("clock",clock_task); else if(kstrcmp(a,"beeper")==0)p=task_spawn("beeper",beeper_task); else if(kstrcmp(a,"counter")==0)p=task_spawn("counter",counter_task); else {kputs("spawn: unknown (clock beeper counter)\n");return;} if(p<0){kputs("spawn: no slot\n");return;} kputs("spawned pid ");print_num((uint32_t)p);kputc('\n'); }
static void cmd_kill(const char *a){ int p=katoi(a); if(p<0||p>=TASK_MAX||!tasks[p].used){kputs("kill: bad pid\n");return;} task_kill(p); kputs("killed pid ");print_num((uint32_t)p);kputc('\n'); }
static void cmd_run(const char *a){ int s=katoi(a); if(s<0||s>3){kputs("run: slot 0..3\n");return;} uint32_t e=elf_load_slot(s); if(!e){kputs("run: no ELF in slot ");print_num((uint32_t)s);kputc('\n');return;} char nm[8]; nm[0]='p';nm[1]='r';nm[2]='o';nm[3]='g';nm[4]=(char)('0'+s);nm[5]=0; int p=task_spawn(nm,(void(*)(void))e); if(p<0){kputs("run: no slot\n");return;} kputs("started pid ");print_num((uint32_t)p);kputs(" at 0x400000\n"); }
static void cmd_wc(void){ int lines=0,chars=0; for(int i=0;i<in_len;i++){chars++;if(in_buf[i]==10)lines++;} kputs("lines=");print_num((uint32_t)lines);kputs(" chars=");print_num((uint32_t)chars);kputc(10); }
static void cmd_grep(const char *pat){ if(!in_buf){kputs("grep: needs stdin (use |)\n");return;} int pl=0; while(pat[pl])pl++; int start=0; for(int i=0;i<=in_len;i++){ if(i==in_len||in_buf[i]==10){ int L=i-start,found=0; for(int s=0;s+pl<=L;s++){int ok=1;for(int k=0;k<pl;k++)if(in_buf[start+s+k]!=pat[k]){ok=0;break;} if(ok){found=1;break;}} if(found){for(int k=start;k<i;k++)kputc(in_buf[k]);kputc(10);} start=i+1; } } }

static void execute_simple(const char *line){
    if(kstrcmp(line,"help")==0)cmd_help();
    else if(kstrcmp(line,"ver")==0)cmd_ver();
    else if(kstrcmp(line,"date")==0)cmd_date();
    else if(kstrcmp(line,"uptime")==0)cmd_uptime();
    else if(kstrncmp(line,"sleep ",6)==0)cmd_sleep(line+6);
    else if(kstrcmp(line,"mem")==0)cmd_mem();
    else if(kstrcmp(line,"ptest")==0)cmd_ptest();
    else if(kstrncmp(line,"malloc ",7)==0)cmd_malloc(line+7);
    else if(kstrncmp(line,"ls",2)==0&&(line[2]==0||line[2]==32))cmd_ls(line[2]==32?line+3:line+2);
    else if(kstrncmp(line,"cat ",4)==0)cmd_cat(line+4);
    else if(kstrncmp(line,"touch ",6)==0)cmd_touch(line+6);
    else if(kstrncmp(line,"rm ",3)==0)cmd_rm(line+3);
    else if(kstrncmp(line,"write ",6)==0)cmd_write(line+6);
    else if(kstrncmp(line,"edit ",5)==0)editor_run(line+5);
    else if(kstrcmp(line,"sync")==0)cmd_sync();
    else if(kstrncmp(line,"tz ",3)==0)cmd_tz(line+3);
    else if(kstrncmp(line,"beep",4)==0)cmd_beep(line+4);
    else if(kstrcmp(line,"panic")==0)cmd_panic();
    else if(kstrcmp(line,"mouse")==0)cmd_mouse();
    else if(kstrcmp(line,"ps")==0)cmd_ps();
    else if(kstrncmp(line,"spawn ",6)==0)cmd_spawn(line+6);
    else if(kstrncmp(line,"kill ",5)==0)cmd_kill(line+5);
    else if(kstrncmp(line,"run ",4)==0)cmd_run(line+4);
    else if(kstrcmp(line,"net")==0)net_cmd_status();
    else if(kstrncmp(line,"ping ",5)==0)net_cmd_ping(line+5);
    else if(kstrcmp(line,"wc")==0)cmd_wc();
    else if(kstrncmp(line,"grep ",5)==0)cmd_grep(line+5);
    else if(kstrcmp(line,"basic")==0)basic_run("");
    else if(kstrncmp(line,"basic ",6)==0)basic_run(line+7);
    else if(kstrncmp(line,"mkdir ",6)==0)cmd_mkdir(line+6);
    else if(kstrncmp(line,"cd ",3)==0)cmd_cd(line+3);
    else if(kstrcmp(line,"pwd")==0)cmd_pwd();
    else if(kstrcmp(line,"clear")==0)vga_clear();
    else if(kstrncmp(line,"echo ",5)==0){kputs(line+5);kputc('\n');}
    else if(kstrcmp(line,"reboot")==0)cmd_reboot();
    else if(kstrlen(line)>0){kputs("Unknown: ");kputs(line);kputs(" (try help)\n");}
}
static void execute(const char *line){
    static char pipe_buf[1024]; int i=0;
    while(line[i]&&line[i]!=124)i++;
    if(line[i]==124){ char left[256]; int n=0; for(int k=0;k<i;k++)left[n++]=line[k]; left[n]=0; while(n>0&&left[n-1]==32)left[--n]=0; const char *right=line+i+1; while(*right==32)right++; out_buf=pipe_buf;out_pos=0;out_max=1024;pipe_buf[0]=0; execute_simple(left); out_buf=0;pipe_buf[out_pos]=0; in_buf=pipe_buf;in_len=out_pos; execute_simple(right); in_buf=0;in_len=0; return; }
    i=0; while(line[i]&&line[i]!=62)i++;
    if(line[i]==62){ char left[256]; int n=0; for(int k=0;k<i;k++)left[n++]=line[k]; left[n]=0; while(n>0&&left[n-1]==32)left[--n]=0; const char *file=line+i+1; while(*file==32)file++; out_buf=pipe_buf;out_pos=0;out_max=1024;pipe_buf[0]=0; execute_simple(left); out_buf=0;pipe_buf[out_pos]=0; fs_write(file,pipe_buf,out_pos); return; }
    i=0; while(line[i]&&line[i]!=60)i++;
    if(line[i]==60){ char left[256]; int n=0; for(int k=0;k<i;k++)left[n++]=line[k]; left[n]=0; while(n>0&&left[n-1]==32)left[--n]=0; const char *file=line+i+1; while(*file==32)file++; static char in_tmp[1024]; int r=fs_read(file,in_tmp,1023); if(r<0){kputs("no such file: ");kputs(file);kputc(10);return;} in_tmp[r]=0; in_buf=in_tmp;in_len=r; execute_simple(left); in_buf=0;in_len=0; return; }
    execute_simple(line);
}
void shell_run(void){
    char line[256]; int pos=0; hist_load();
    kputs("\n========================================\n");
    kputs("  NexusShell 1.0 - type 'help'\n");
    kputs("  Up/Down = history, run S = ELF slot\n");
    kputs("========================================\n\n");
    for(;;){ kputs("nexus> "); pos=0; line[0]=0;
        for(;;){ uint8_t sc=keyboard_getscan(); if(sc&0x80)continue;
            if(sc==0x48){ if(hist_pos+1<hist_count){ hist_pos++; while(pos>0){pos--;kputs("\b \b");} for(int k=0;hist[hist_pos][k]&&k<254;k++){line[pos++]=hist[hist_pos][k];kputc(hist[hist_pos][k]);} line[pos]=0; } continue; }
            if(sc==0x50){ if(hist_pos>=0){ hist_pos--; const char *s=(hist_pos>=0)?hist[hist_pos]:""; while(pos>0){pos--;kputs("\b \b");} for(int k=0;s[k]&&k<254;k++){line[pos++]=s[k];kputc(s[k]);} line[pos]=0; } continue; }
            if(sc==0x1C){ kputc('\n'); line[pos]=0; hist_push(line); hist_save(); execute(line); break; }
            if(sc==0x0E){ if(pos>0){pos--;kputs("\b \b");} continue; }
            char c=keyboard_ascii(sc); if(c&&c!='\t'&&pos<254){line[pos++]=c;kputc(c);}
        }
    }
}
