#ifndef KERNEL_VGA_H
#define KERNEL_VGA_H
#include <stdint.h>
enum vga_color {
    VGA_BLACK=0,VGA_BLUE=1,VGA_GREEN=2,VGA_CYAN=3,VGA_RED=4,VGA_MAGENTA=5,
    VGA_BROWN=6,VGA_LGREY=7,VGA_DGREY=8,VGA_LBLUE=9,VGA_LGREEN=10,VGA_LCYAN=11,
    VGA_LRED=12,VGA_LMAGENTA=13,VGA_LBROWN=14,VGA_WHITE=15
};
void vga_init(void);
void vga_clear(void);
void vga_setcolor(uint8_t fg, uint8_t bg);
void vga_putchar(char c);
void vga_puts(const char *s);
void vga_putc_at(int x,int y,char c,uint8_t fg,uint8_t bg);
uint16_t vga_get_at(int x,int y);
void vga_put_at(int x,int y,uint16_t v);
#endif
