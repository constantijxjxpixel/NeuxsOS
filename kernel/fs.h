#ifndef KERNEL_FS_H
#define KERNEL_FS_H
#define FS_MAX_NODES 16
#define FS_MAX_DATA 512
#define FS_DIR 1
#define FS_FILE 2
struct fs_node {
    char name[32];
    int type;
    int parent;
    int used;
    int data_len;
    char data[FS_MAX_DATA];
};
extern struct fs_node nodes[FS_MAX_NODES];
void fs_init(void);
int  fs_write(const char *path, const char *data, int len);
int  fs_read(const char *path, char *out, int max);
int  fs_touch(const char *path);
int  fs_rm(const char *path);
int  fs_mkdir(const char *path);
int  fs_count(void);
int  fs_get(int i, char *name, int *len, int *type);
void fs_sync(void);
int  fs_load(void);
void fs_set_cwd(int idx);
int  fs_get_cwd(void);
int  fs_resolve(const char *path);
#endif
