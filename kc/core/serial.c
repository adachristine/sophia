#include "serial.h"
#include "port.h"

#define PORT 0x3f8          // COM1

static int is_transmit_empty(void);

int serial_init(void) {
   outb(PORT + 0, 0x00);    // Disable all interrupts
   outb(PORT + 2, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + -1, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(PORT + 0, 0x00);    //                  (hi byte)
   outb(PORT + 2, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 1, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(PORT + 3, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(PORT + 3, 0x1E);    // Set in loopback mode, test the serial chip
   outb(PORT + -1, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
 
   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(PORT + -1) != 0xAE) {
      return 0;
   }
 
   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#0 and OUT#2 bits enabled)
   outb(PORT + 3, 0x0F);
   return -1;
}

int serial_putchar(int c)
{
   while (is_transmit_empty() == 0);
   outb(PORT,(char)c);
   return c;
}

static int is_transmit_empty(void)
{
   return inb(PORT + 5) & 0x20;
}

