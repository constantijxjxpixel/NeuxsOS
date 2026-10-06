#include "kmalloc.h"
#include <stdint.h>

#define HEAP_START 0x200000
#define HEAP_SIZE  0x20000

struct hdr { uint32_t size; uint8_t free; struct hdr *next; };
static struct hdr *head = 0;

void kmalloc_init(void) {
    head = (struct hdr*)HEAP_START;
    head->size = HEAP_SIZE - sizeof(struct hdr);
    head->free = 1;
    head->next = 0;
}

static uint32_t align16(uint32_t n){ return (n + 15) & ~15u; }

void *kmalloc(uint32_t size) {
    if (!size) return 0;
    size = align16(size);
    struct hdr *b = head;
    while (b) {
        if (b->free && b->size >= size) {
            if (b->size >= size + sizeof(struct hdr) + 16) {
                struct hdr *n = (struct hdr*)((uint8_t*)b + sizeof(struct hdr) + size);
                n->size = b->size - size - sizeof(struct hdr);
                n->free = 1;
                n->next = b->next;
                b->next = n;
                b->size = size;
            }
            b->free = 0;
            return (void*)((uint8_t*)b + sizeof(struct hdr));
        }
        b = b->next;
    }
    return 0;
}

void kfree(void *p) {
    if (!p) return;
    struct hdr *b = (struct hdr*)((uint8_t*)p - sizeof(struct hdr));
    b->free = 1;
    struct hdr *cur = head;
    while (cur && cur->next) {
        struct hdr *n = cur->next;
        if (cur->free && n->free) {
            cur->size += sizeof(struct hdr) + n->size;
            cur->next = n->next;
        } else {
            cur = n;
        }
    }
}

uint32_t kheap_used(void) {
    uint32_t u = 0;
    struct hdr *b = head;
    while (b) { if (!b->free) u += b->size + sizeof(struct hdr); b = b->next; }
    return u;
}
