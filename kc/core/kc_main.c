#include "cpu.h"
#include "serial.h"
#include "kprint.h"
#include "memory.h"
#include "panic.h"

#include <kernel/entry.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

unsigned long kc_main(struct kc_boot_data *data)
{
    (void)data;
    cpu_init();
    serial_init();
    kputs("<hacker voice> i'm in 3333\r\n");
    memory_init(data);
    halt();
}

