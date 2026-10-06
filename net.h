#ifndef KERNEL_NET_H
#define KERNEL_NET_H
#include <stdint.h>
int  net_install(void);
void net_poll(void);
void net_cmd_status(void);
int  net_cmd_ping(const char *args);
#endif
