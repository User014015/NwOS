#include "io.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct __attribute__((packed)) {
    u16 base_lo;
    u16 sel;
    u8  always0;
    u8  flags;
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
extern void irq0_stub(void);

static void set_gate(int i, u32 base) {
    idt[i].base_lo = (u16)(base & 0xFFFF);
    idt[i].sel     = 0x08;
    idt[i].always0 = 0;
    idt[i].flags   = 0x8E;
    idt[i].base_hi = (u16)((base >> 16) & 0xFFFF);
}

void idt_init(void) {
    for (int i = 0; i < 256; i++) set_gate(i, (u32)isr_default);
    set_gate(0x20, (u32)irq0_stub);

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (u32)&idt;
    idt_load(&idt_ptr);

    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}