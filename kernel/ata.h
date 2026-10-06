#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H
#include <stdint.h>
void ata_read_sectors(uint32_t lba, uint8_t count, uint8_t *buf);
void ata_write_sectors(uint32_t lba, uint8_t count, const uint8_t *buf);
#endif
