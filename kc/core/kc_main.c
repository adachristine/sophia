#include "cpu.h"
#include "serial.h"
#include "kprint.h"
#include "memory.h"
#include "panic.h"
#include "task.h"

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
    kputs("<hacker voice> i'm in 3333\r\n");
    memory_init();
    task_init();

    return 0;
}

struct kc_boot_data *get_boot_data()
{
    return boot_data;
}

