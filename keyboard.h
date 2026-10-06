#ifndef KERNEL_KEYBOARD_H
#define KERNEL_KEYBOARD_H
#include <stdint.h>
void keyboard_install(void);
char keyboard_getchar(void);
uint8_t keyboard_getscan(void);
int  keyboard_scan_pending(void);
void keyboard_flush(void);
char keyboard_ascii(uint8_t sc);
int  keyboard_kbhit(void);
#endif
