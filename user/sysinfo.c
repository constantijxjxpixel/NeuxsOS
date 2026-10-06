#include "user.h"
int main(void){
    unsigned t = sys_ticks();
    char rev[12]; int r=0;
    if (!t) rev[r++]='0';
    while (t){ rev[r++]=(char)('0'+(t%10)); t/=10; }
    char b[13]; int i=0;
    while (r) b[i++]=rev[--r];
    b[i]=0;
    sys_write("kernel ticks = ");
    sys_write(b);
    sys_write("\n");
    return 0;
}
