#include "splash.h"
#include "vga.h"
#include "timer.h"

static const char *art[17] = {
    "    .----------------------------.    ",
    "   /                              \\   ",
    "  |    /|                  |\\    |  ",
    "  |   / |\\                /| \\   |  ",
    "  |  /  | \\              / |  \\  |  ",
    "  | /   |  \\____________/  |   \\ |  ",
    "  | |   |  /            \\  |   | |  ",
    "  | \\   | |  []      []  | |   / |  ",
    "  |  \\  | |      __      | |  /  |  ",
    "  |   \\ | |     /  \\     | | /   |  ",
    "  |    \\| |     \\__/     | |/    |  ",
    "  |     | \\              / |     |  ",
    "  |     |  \\____________/  |     |  ",
    "  |      \\   /\\/\\/\\/\\/\\   /      |  ",
    "  |       \\______________/       |  ",
    "   \\                              /   ",
    "    '----------------------------'    "
};

void splash_show(void){
    vga_clear();
    for (int r=0; r<17; r++){
        uint8_t base = (r<=1 || r>=15) ? VGA_LCYAN : VGA_LGREY;
        for (int i=0; art[r][i]; i++){
            char c = art[r][i];
            uint8_t fg = (c=='[' || c==']') ? VGA_LRED : base;
            vga_putc_at(21+i, 2+r, c, fg, VGA_BLACK);
        }
    }
    const char *name = "N e x u s O S";
    for (int i=0; name[i]; i++) vga_putc_at(33+i, 20, name[i], VGA_LRED, VGA_BLACK);
    const char *tag = "Code & Productivity in One Core";
    for (int i=0; tag[i]; i++) vga_putc_at(24+i, 21, tag[i], VGA_DGREY, VGA_BLACK);
    const char *ed = "Stage 8: persistence + comfort";
    for (int i=0; ed[i]; i++) vga_putc_at(25+i, 22, ed[i], VGA_DGREY, VGA_BLACK);
    timer_wait_ms(2200);
}
