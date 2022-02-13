#include "pit8253.h"
#include "pic8259.h"
#include "port.h"

#define PIT8253_DATA0 0x40
#define PIT8253_CMD 0x43

#define SEL_CHANNEL0 0x0
#define ACC_LOHI 0x30
#define MODE_RATE 0x04
#define BIN_COUNT 0x0

#define CONFIG_CHANNEL_0 (SEL_CHANNEL0|ACC_LOHI|MODE_RATE|BIN_COUNT)

void pit8253_init(void)
{
    outb(PIT8253_CMD, CONFIG_CHANNEL_0);
    outb(PIT8253_DATA0, 0xff);
    outb(PIT8253_DATA0, 0xff);

    pic8259_irq_unmask(0);
}

void pit8253_start(void)
{
}

void pit8253_stop(void)
{
}

void pit8253_set_hz(uint16_t hz)
{
    (void)hz;
}

