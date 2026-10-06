#ifndef KERNEL_KMALLOC_H
#define KERNEL_KMALLOC_H
#include <stdint.h>
void kmalloc_init(void);
void *kmalloc(uint32_t size);
void kfree(void *p);
uint32_t kheap_used(void);
#endif
