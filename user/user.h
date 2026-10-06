#ifndef USER_H
#define USER_H
static inline void sys_write(const char *s){ __asm__ volatile("int $0x80" :: "a"(1), "b"(s) : "memory"); }
static inline void sys_exit(void){ __asm__ volatile("int $0x80" :: "a"(2)); }
static inline void sys_beep(unsigned f){ __asm__ volatile("int $0x80" :: "a"(3), "b"(f)); }
static inline unsigned sys_ticks(void){ unsigned r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(4)); return r; }
#endif
