#include "types.h"

typedef struct __attribute__((packed)) {
    u16 base_lo, sel;
    u8  always0, flags;
    u16 base_hi;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    u16 limit;
    u32 base;
} idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr;

extern void idt_load(idt_ptr_t *p);
extern void isr_default(void);

void idt_init(void) {
    u32 base = (u32)isr_default;
    for (int i = 0; i < 256; i++) {
        idt[i].base_lo = (u16)(base & 0xFFFF);
        idt[i].sel     = 0x08;
        idt[i].always0 = 0;
        idt[i].flags   = 0x8E;
        idt[i].base_hi = (u16)((base >> 16) & 0xFFFF);
    }
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (u32)&idt;
    idt_load(&idt_ptr);
}