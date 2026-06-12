/* ============================================================================
 * kernel/mouse.c — PS/2 Mouse Driver with IntelliMouse Extensions
 *
 * Purpose : Handles PS/2 mouse input on IRQ12. Specifically activates the
 *           scroll wheel (Z-axis) via a magic init sequence. When the wheel
 *           is scrolled, it triggers the terminal scrolling functions.
 * ============================================================================ */

#include "../include/mouse.h"
#include "../include/pic.h"
#include "../include/idt.h"

/* Terminal scroll API */
extern void terminal_scroll_up(int lines);
extern void terminal_scroll_down(int lines);

static void mouse_wait(uint8_t type) {
    int timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return; /* Data available */
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return; /* Ready to write */
        }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4); /* Tell controller to send command to mouse */
    mouse_wait(1);
    outb(0x60, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

static void mouse_write_ack(uint8_t data) {
    mouse_write(data);
    mouse_read(); /* Read ACK 0xFA */
}

static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[4];
static uint8_t mouse_packet_size = 3; /* Defaults to 3 unless IntelliMouse is verified */

static void mouse_irq_handler(registers_t *regs) {
    (void)regs;
    
    /* Ensure there's data from the mouse */
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return; 
    
    uint8_t data = inb(0x60);

    /* Sync check */
    if (mouse_cycle == 0 && !(data & 0x08)) return;

    mouse_packet[mouse_cycle] = data;
    mouse_cycle++;

    if (mouse_cycle == mouse_packet_size) {
        mouse_cycle = 0;
        
        if (mouse_packet_size == 4) {
            /* Z-axis is in the 4th byte. Positive = scroll down, negative = scroll up */
            int8_t z = (int8_t)mouse_packet[3];
            if (z > 0) {
                terminal_scroll_down(3);
            } else if (z < 0) {
                terminal_scroll_up(3);
            }
        }
    }
}

void mouse_install(void) {
    /* 1. Enable auxiliary mouse device */
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    /* 2. Modify Compaq Status byte */
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = inb(0x60);
    status |= 2;       /* Enable IRQ12 */
    status &= ~0x20;   /* Clear Disable Mouse Clock bit */
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    
    /* 3. Set to default */
    mouse_write_ack(0xF6);
    
    /* 4. Enable IntelliMouse Extensions (Scroll Wheel) */
    mouse_write_ack(0xF3);
    mouse_write_ack(200);
    mouse_write_ack(0xF3);
    mouse_write_ack(100);
    mouse_write_ack(0xF3);
    mouse_write_ack(80);
    
    /* Check Device ID to see if IntelliMouse mode was accepted */
    mouse_write_ack(0xF2);
    uint8_t mouse_id = mouse_read();
    if (mouse_id == 3) {
        mouse_packet_size = 4;
    } else {
        mouse_packet_size = 3;
    }
    
    /* 5. Enable data reporting */
    mouse_write_ack(0xF4);
    
    /* 6. Register IRQ12 handler (Vector 44) */
    idt_set_handler(44, mouse_irq_handler);
    pic_unmask_irq(12); /* Unmask IRQ12 on the slave PIC */
    pic_unmask_irq(2);  /* Unmask IRQ2 on the master PIC (cascade line) */
}
