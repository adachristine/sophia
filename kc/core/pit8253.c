#include "pit8253.h"
#include "pic8259.h"
#include "port.h"
#include "kprint.h"

#include <stdatomic.h>

#define PIT8253_DATA0 0x40
#define PIT8253_CMD 0x43

#define SEL_CHANNEL0 0x0
#define ACC_LOHI 0x30
#define MODE_RATE 0x04
#define BIN_COUNT 0x0

#define CONFIG_CHANNEL_0 (SEL_CHANNEL0|ACC_LOHI|MODE_RATE|BIN_COUNT)

#define PIT_IRQ 0
#define PIT_HZ 1193182U
#define PIT_HZ_MIN (PIT_HZ/0xffff)

static void pit8253_set_divisor(uint16_t divisor);

static atomic_uint_fast64_t count = 0;
static uint16_t divisor = 0;

const struct timer_source pit8253_timer_source =
{
    "pit8253",
    pit8253_init,
    pit8253_start,
    pit8253_stop,

    pit8253_set_frequency,
    pit8253_get_frequency,

    pit8253_nanoseconds_elapsed,
    pit8253_nanoseconds_delta
};

int pit8253_callback(uint8_t irq)
{
    (void)irq;
    count++;

    return 0;
}

void pit8253_init(void)
{
    pic8259_irq_install(PIT_IRQ, pit8253_callback);
}

void pit8253_start(void)
{
    pic8259_irq_unmask(PIT_IRQ);
}

void pit8253_stop(void)
{
    pic8259_irq_mask(PIT_IRQ);
}

void pit8253_set_frequency(uint32_t frequency)
{
    uint32_t d = PIT_HZ / frequency;

    if (d > UINT16_MAX)
    {
        kprintf("warning: %uHz is too slow for 8253 pit\n", PIT_HZ/d);
        d = UINT16_MAX;
        kprintf("warning: set timer to minimum frequency\n");
    }

    pit8253_set_divisor(d);
}

uint32_t pit8253_get_frequency(void)
{
    return PIT_HZ/divisor;
}

uint64_t pit8253_nanoseconds_elapsed(void)
{
    return pit8253_nanoseconds_delta() * count;
}

uint64_t pit8253_nanoseconds_delta(void)
{
    return (TIMER_NANOSECOND * divisor)/PIT_HZ;
}

static void pit8253_set_divisor(uint16_t d)
{
    outb(PIT8253_CMD, CONFIG_CHANNEL_0);
    outb(PIT8253_DATA0, (uint8_t)d);
    outb(PIT8253_DATA0, (uint8_t)(d >> 8));
    divisor = d;
}

