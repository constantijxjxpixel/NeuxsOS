#include "user.h"
int main(void){
    sys_write("=== MyProgram v1.0 ===\n");
    for (int i = 3; i > 0; i--) {
        sys_write("countdown: ");
        char b[2]; b[0] = (char)(48 + i); b[1] = 0;
        sys_write(b);
        sys_write("\n");
        sys_beep(440 + (unsigned)(3 - i) * 220);
        unsigned t0 = sys_ticks();
        while (sys_ticks() - t0 < 100) { }
    }
    sys_write("LIFTOFF!\n");
    sys_beep(1760);
    return 0;
}
