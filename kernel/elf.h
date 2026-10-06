#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H
#include <stdint.h>
void elf_install(void);
uint32_t elf_load_slot(int slot);
#endif
