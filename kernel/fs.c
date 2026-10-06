#include "fs.h"
#include "ata.h"
#include <stdint.h>

#define FS_LBA 2048
#define FS_SECTORS 20

struct fs_node nodes[FS_MAX_NODES];
static uint8_t fs_buf[FS_SECTORS*512];
static int cwd = 0;

static int kstrlen(const char *s){ int l=0; while(s[l]) l++; return l; }
static int name_eq(const char *a, const char *b){ while(*a&&*b){if(*a!=*b)return 0;a++;b++;} return *a==*b; }

static int find_child(int parent, const char *name){
    for(int i=0;i<FS_MAX_NODES;i++)
        if(nodes[i].used && nodes[i].parent==parent && name_eq(nodes[i].name,name)) return i;
    return -1;
}

static int create_node(int parent, const char *name, int type){
    for(int i=0;i<FS_MAX_NODES;i++){
        if(!nodes[i].used){
            nodes[i].used=1; nodes[i].type=type; nodes[i].parent=parent; nodes[i].data_len=0;
            int k=0; while(name[k]&&k<31){nodes[i].name[k]=name[k];k++;} nodes[i].name[k]=0;
            return i;
        }
    }
    return -1;
}

/* split "a/b/c" -> prefix "a/b", name "c". no slash -> prefix "", name whole */
static void split_path(const char *path, char *prefix, char *name){
    const char *last = 0;
    for(const char *p=path;*p;p++) if(*p==47) last=p;
    if(!last){ prefix[0]=0; int k=0; while(path[k]&&k<31){name[k]=path[k];k++;} name[k]=0; return; }
    int pl = (int)(last-path);
    if(pl>255) pl=255;
    for(int k=0;k<pl;k++) prefix[k]=path[k];
    prefix[pl]=0;
    const char *n = last+1;
    int k=0; while(n[k]&&k<31){name[k]=n[k];k++;} name[k]=0;
}

void fs_init(void){
    for(int i=0;i<FS_MAX_NODES;i++){ nodes[i].used=0; nodes[i].data_len=0; nodes[i].name[0]=0; }
    nodes[0].used=1; nodes[0].type=FS_DIR; nodes[0].parent=0; nodes[0].data_len=0;
    int k=0; while("root"[k]&&k<31){nodes[0].name[k]="root"[k];k++;} nodes[0].name[k]=0;
    int home = create_node(0, "home", FS_DIR);
    int user = create_node(home, "user", FS_DIR);
    fs_write("/home/user/readme.txt", "Welcome to NexusOS 1.0!\nFiles persist after sync + reboot.\n", -1);
    fs_write("/home/user/motd.txt", "Code & Productivity in One Core.\n", -1);
    fs_write("/home/user/todo.txt", "[x] stages 1-10\n[ ] FAT fs\n[ ] ring 3\n", -1);
    cwd = 0;
}

int fs_resolve(const char *path){
    if(!path || !path[0]) return cwd;
    int cur = (path[0]==47) ? 0 : cwd;
    const char *p = path;
    if(*p==47) p++;
    while(*p){
        const char *next = p;
        while(*next && *next!=47) next++;
        int len = next-p;
        char seg[33]; int k=0; while(k<len && k<32){seg[k]=p[k];k++;} seg[k]=0;
        if(name_eq(seg,".")) { }
        else if(name_eq(seg,"..")) { cur = nodes[cur].parent; }
        else {
            int child = find_child(cur, seg);
            if(child<0) return -1;
            cur = child;
        }
        p = next;
        if(*p==47) p++;
    }
    return cur;
}

int fs_write(const char *path, const char *data, int len){
    char prefix[256], name[32];
    split_path(path, prefix, name);
    if(!name[0]) return -1;
    int parent = prefix[0] ? fs_resolve(prefix) : cwd;
    if(parent<0) return -1;
    int idx = find_child(parent, name);
    if(idx<0){ idx = create_node(parent, name, FS_FILE); if(idx<0) return -1; }
    else if(nodes[idx].type!=FS_FILE) return -1;
    if(len<0) len = kstrlen(data);
    if(len>FS_MAX_DATA) len = FS_MAX_DATA;
    for(int k=0;k<len;k++) nodes[idx].data[k]=data[k];
    nodes[idx].data_len = len;
    return len;
}

int fs_read(const char *path, char *out, int max){
    int idx = fs_resolve(path);
    if(idx<0 || nodes[idx].type!=FS_FILE) return -1;
    if(!out) return nodes[idx].data_len;
    int n = nodes[idx].data_len; if(n>max) n=max;
    for(int k=0;k<n;k++) out[k]=nodes[idx].data[k];
    return n;
}

int fs_touch(const char *path){
    char prefix[256], name[32];
    split_path(path, prefix, name);
    if(!name[0]) return -1;
    int parent = prefix[0] ? fs_resolve(prefix) : cwd;
    if(parent<0) return -1;
    if(find_child(parent,name)>=0) return 0;
    return create_node(parent, name, FS_FILE) >= 0 ? 0 : -1;
}

int fs_rm(const char *path){
    int idx = fs_resolve(path);
    if(idx<0) return -1;
    for(int i=0;i<FS_MAX_NODES;i++) if(nodes[i].used && nodes[i].parent==idx) return -1;
    nodes[idx].used=0;
    return 0;
}

int fs_mkdir(const char *path){
    char prefix[256], name[32];
    split_path(path, prefix, name);
    if(!name[0]) return -1;
    int parent = prefix[0] ? fs_resolve(prefix) : cwd;
    if(parent<0) return -1;
    if(find_child(parent,name)>=0) return -1;
    return create_node(parent, name, FS_DIR) >= 0 ? 0 : -1;
}

int fs_count(void){ int n=0; for(int i=0;i<FS_MAX_NODES;i++) if(nodes[i].used) n++; return n; }

int fs_get(int i, char *name, int *len, int *type){
    int cnt=0;
    for(int k=0;k<FS_MAX_NODES;k++){
        if(!nodes[k].used) continue;
        if(cnt==i){
            int j=0; while(nodes[k].name[j]){name[j]=nodes[k].name[j];j++;} name[j]=0;
            *len = nodes[k].data_len; *type = nodes[k].type; return 1;
        }
        cnt++;
    }
    return 0;
}

void fs_set_cwd(int idx){ cwd = idx; }
int fs_get_cwd(void){ return cwd; }

void fs_sync(void){
    int off=0;
    fs_buf[off++]='N'; fs_buf[off++]='X'; fs_buf[off++]='F'; fs_buf[off++]='S';
    fs_buf[off++]=(uint8_t)(cwd&0xFF); fs_buf[off++]=(uint8_t)((cwd>>8)&0xFF);
    for(int i=0;i<FS_MAX_NODES;i++){
        fs_buf[off++]=(uint8_t)nodes[i].used;
        fs_buf[off++]=(uint8_t)nodes[i].type;
        fs_buf[off++]=(uint8_t)(nodes[i].parent&0xFF); fs_buf[off++]=(uint8_t)((nodes[i].parent>>8)&0xFF);
        for(int k=0;k<32;k++) fs_buf[off++]=(uint8_t)nodes[i].name[k];
        fs_buf[off++]=(uint8_t)(nodes[i].data_len&0xFF); fs_buf[off++]=(uint8_t)((nodes[i].data_len>>8)&0xFF);
        fs_buf[off++]=0; fs_buf[off++]=0;
        for(int k=0;k<FS_MAX_DATA;k++) fs_buf[off++]=(uint8_t)nodes[i].data[k];
    }
    ata_write_sectors(FS_LBA, FS_SECTORS, fs_buf);
}

int fs_load(void){
    ata_read_sectors(FS_LBA, FS_SECTORS, fs_buf);
    if(fs_buf[0]!='N'||fs_buf[1]!='X'||fs_buf[2]!='F'||fs_buf[3]!='S') return 0;
    int off=4;
    cwd = (int)fs_buf[off] | ((int)fs_buf[off+1]<<8); off+=2;
    for(int i=0;i<FS_MAX_NODES;i++){
        nodes[i].used = fs_buf[off++];
        nodes[i].type = fs_buf[off++];
        nodes[i].parent = (int)fs_buf[off] | ((int)fs_buf[off+1]<<8); off+=2;
        for(int k=0;k<32;k++) nodes[i].name[k]=(char)fs_buf[off++];
        nodes[i].data_len = (int)fs_buf[off] | ((int)fs_buf[off+1]<<8); off+=4;
        for(int k=0;k<FS_MAX_DATA;k++) nodes[i].data[k]=(char)fs_buf[off++];
    }
    return 1;
}
