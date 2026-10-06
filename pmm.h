#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H
#include <stdint.h>
uint32_t detect_ram_bytes(void);
void paging_init(void);
void pmm_init(uint32_t total_bytes);
uint32_t pmm_alloc_page(void);
void pmm_free_page(uint32_t addr);
uint32_t pmm_total_pages(void);
uint32_t pmm_free_pages(void);
#endif
