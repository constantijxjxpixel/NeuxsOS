#include "vga.h"

static uint16_t *const buffer = (uint16_t*)0xB8000;
static int row = 0, col = 0;
static uint8_t color = 0x07;

static inline uint16_t vc(char c, uint8_t fg, uint8_t bg){ return (uint16_t)((bg<<12)|(fg<<8)|(uint8_t)c); }

void vga_set_color(uint8_t fg, uint8_t bg){ color = (uint8_t)((bg<<4)|fg); }

void vga_clear(void){
    for(int i=0;i<80*25;i++) buffer[i]=vc(' ',VGA_LGREY,VGA_BLACK);
    row=0; col=0;
}

static void scroll(void){
    for(int r=0;r<24;r++) for(int c=0;c<80;c++) buffer[r*80+c]=buffer[(r+1)*80+c];
    for(int c=0;c<80;c++) buffer[24*80+c]=vc(' ',VGA_LGREY,VGA_BLACK);
    row=24;
}

void vga_putchar(char c){
    if(c==10){ col=0; row++; if(row>24) scroll(); return; }
    if(c==13){ col=0; return; }
    if(c==8){ if(col>0) col--; else if(row>0){ row--; col=79; } return; }
    buffer[row*80+col]=vc(c,(uint8_t)(color&0x0F),(uint8_t)(color>>4));
    col++;
    if(col==80){ col=0; row++; if(row>24) scroll(); }
}

void vga_puts(const char *s){ while(*s) vga_putchar(*s++); }

void vga_putc_at(int x,int y,char c,uint8_t fg,uint8_t bg){
    if(x<0||x>79||y<0||y>24) return;
    buffer[y*80+x]=vc(c,fg,bg);
}

uint16_t vga_get_at(int x,int y){ return buffer[y*80+x]; }
void vga_put_at(int x,int y,uint16_t v){ buffer[y*80+x]=v; }

void vga_init(void){
    vga_set_color(VGA_LGREY,VGA_BLACK);
    vga_clear();
}
