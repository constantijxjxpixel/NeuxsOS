#include "editor.h"
#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include <stdint.h>

static char ebuf[512];
static int elen=0, ecur=0;
static char esaved_msg=0;

static int kstrlen(const char *s){ int l=0; while(s[l]) l++; return l; }

static void put_str_at(int x,int y,const char *s,uint8_t fg,uint8_t bg){
    for(int i=0;s[i];i++) vga_putc_at(x+i,y,s[i],fg,bg);
}

static void line_col(int idx,int *ol,int *oc){
    int l=0,c=0;
    for(int i=0;i<idx;i++){ if(ebuf[i]=='\n'){l++;c=0;} else c++; }
    *ol=l; *oc=c;
}
static int idx_of(int line,int col){
    int l=0,c=0;
    for(int i=0;i<=elen;i++){
        if(i==elen) return elen;
        if(l>line) return i;
        if(l==line && c==col) return i;
        if(ebuf[i]=='\n'){ if(l==line) return i; l++; c=0; }
        else c++;
    }
    return elen;
}

static void ed_render(const char *fname){
    vga_clear();
    int x=0,y=0;
    for(int i=0;i<elen;i++){
        char c=ebuf[i];
        if(c=='\n'){ x=0; y++; if(y>22)y=22; continue; }
        if(x>=79){ x=0; y++; if(y>22)y=22; }
        vga_putc_at(x,y,c,VGA_LGREY,VGA_BLACK);
        x++;
    }
    int cl,cc; line_col(ecur,&cl,&cc);
    char under = (ecur<elen)?ebuf[ecur]:' ';
    vga_putc_at(cc,cl,under,VGA_BLACK,VGA_LGREY);
    put_str_at(0,24," F2=save  ESC=quit  ",VGA_WHITE,VGA_BLUE);
    put_str_at(20,24,fname,VGA_LCYAN,VGA_BLUE);
    char info[24]; int n=elen, k=0;
    info[k++]=' '; info[k++]='(';
    if(!n) info[k++]='0';
    int t=n; char rev[8]; int r=0; while(t){rev[r++]='0'+(t%10); t/=10;} while(r) info[k++]=rev[--r];
    info[k++]='b'; info[k++]=')'; info[k++]=0;
    put_str_at(20+kstrlen(fname),24,info,VGA_WHITE,VGA_BLUE);
    if (esaved_msg){ put_str_at(60,24,"[SAVED]",VGA_LGREEN,VGA_BLUE); esaved_msg=0; }
}

static void ed_insert(char c){
    if (elen >= 510) return;
    for(int i=elen;i>ecur;i--) ebuf[i]=ebuf[i-1];
    ebuf[ecur++]=c; elen++;
}

void editor_run(const char *name){
    elen = fs_read(name, ebuf, 511);
    if (elen < 0) elen = 0;
    ecur = 0;
    keyboard_flush();
    ed_render(name);
    for(;;){
        uint8_t sc = keyboard_getscan();
        if (sc & 0x80) continue;
        if (sc == 0x01) break;
        if (sc == 0x3C) { fs_write(name, ebuf, elen); esaved_msg=1; ed_render(name); continue; }
        if (sc == 0x0E) { if(ecur>0){ for(int i=ecur-1;i<elen;i++) ebuf[i]=ebuf[i+1]; elen--; ecur--; } }
        else if (sc == 0x4B) { if(ecur>0) ecur--; }
        else if (sc == 0x4D) { if(ecur<elen) ecur++; }
        else if (sc == 0x48) { int l,c; line_col(ecur,&l,&c); if(l>0) ecur=idx_of(l-1,c); }
        else if (sc == 0x50) { int l,c; line_col(ecur,&l,&c); ecur=idx_of(l+1,c); }
        else if (sc == 0x1C) { ed_insert('\n'); }
        else {
            char c = keyboard_ascii(sc);
            if (c && c != '\t') ed_insert(c);
        }
        ed_render(name);
    }
    keyboard_flush();
    vga_clear();
}
