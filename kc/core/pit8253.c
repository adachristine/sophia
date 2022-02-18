#include "pit8253.h"
#include "pic8259.h"
#include "port.h"
#include "kprint.h"
#include "memory.h"

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

static void set_divisor(uint16_t divisor);

static volatile atomic_uint_fast64_t count = 0;
static uint16_t divisor = 0;
static struct timer_callback_list *callback_list;

const struct timer_source pit8253_timer_source =
{
    "pit8253",
    pit8253_init,
    pit8253_start,
    pit8253_stop,

    pit8253_set_frequency,
    pit8253_get_frequency,

    pit8253_append_callback,
    pit8253_delete_callback,

    pit8253_nanoseconds_elapsed,
    pit8253_nanoseconds_delta
};

int pit8253_callback(uint8_t irq)
{
    (void)irq;
    count++;

    struct timer_callback_list *l = callback_list;
    uint64_t time_now = pit8253_nanoseconds_elapsed();

    while (l)
    {
        l->func(time_now);
        l = l->next;
    }

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

    set_divisor(d);
}

uint32_t pit8253_get_frequency(void)
{
    return PIT_HZ/divisor;
}

static struct timer_callback_list *find_callback(timer_callback func)
{
    struct timer_callback_list *l = callback_list;

    while (l)
    {
        if (l->func == func)
        {
            return l;
        }

        l = l->next;
    }

    return NULL;
}

int pit8253_append_callback(timer_callback func)
{
    if (!find_callback(func))
    {
        struct timer_callback_list *item = heap_alloc(sizeof(*item));
        item->func = func;
        item->next = callback_list;
        callback_list = item;

        return 0;
    }

    return -1;
}

void pit8253_delete_callback(timer_callback func)
{
    // TODO: implement this
    (void)func;
}

uint64_t pit8253_nanoseconds_elapsed(void)
{
    return pit8253_nanoseconds_delta() * atomic_load(&count);
}

uint64_t pit8253_nanoseconds_delta(void)
{
    return (TIMER_NANOSECOND * divisor)/PIT_HZ;
}

static void set_divisor(uint16_t d)
{
    outb(PIT8253_CMD, CONFIG_CHANNEL_0);
    outb(PIT8253_DATA0, (uint8_t)d);
    outb(PIT8253_DATA0, (uint8_t)(d >> 8));
    divisor = d;
}

