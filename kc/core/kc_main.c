#include "cpu.h"
#include "serial.h"
#include "kprint.h"
#include "memory.h"
#include "panic.h"
#include "task.h"
#include "pic8259.h"
#include "pit8253.h"
#include "cpu/irq.h"

#include <kernel/entry.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//TODO: initialize vm before cpu?
//      alternatively, split memory and/or cpu initialization steps

static struct kc_boot_data *boot_data;

unsigned long kc_main(struct kc_boot_data *data)
{
    boot_data = data;
    cpu_init();
    serial_init();
    kprintf("sophia starting, boot data %#lx\n", boot_data);
    memory_init();
    pic8259_init();
    pit8253_timer_source.init();
    pit8253_timer_source.set_frequency(1);
    task_init();

    return 0;
}

struct kc_boot_data *get_boot_data()
{
    return boot_data;
}

