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

unsigned long kc_main(struct kc_boot_data *data)
{
    cpu_init();
    serial_init();
    kputs("<hacker voice> i'm in 3333\r\n");
    memory_init(data);
    task_init();

    return 0;
}

