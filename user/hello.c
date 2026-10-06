#include "user.h"
int main(void){
    sys_write("Hello from user program!\n");
    sys_write("ELF loader + syscalls work.\n");
    sys_beep(880);
    sys_write("Beep done. Exiting via syscall 2.\n");
    return 0;
}
