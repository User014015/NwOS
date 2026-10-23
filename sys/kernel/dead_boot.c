#include "fb.h"
#include "../../include/LogoNwOS.h"

void clear_screen_boot(void) {
    volatile uint16_t *vga = (uint16_t *)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = ' ' | (0x07 << 8);
    }
}

void dead_boot(void) {
    clear_screen_boot();
    
    if (fb_is_available()) {
        fb_draw_image(((int)fb_get_width() - LOGO_NWOS_WIDTH) / 2, 40, LogoNwOS, LOGO_NWOS_WIDTH, LOGO_NWOS_HEIGHT);
    }
    
    while (1) __asm__ volatile("hlt");
}