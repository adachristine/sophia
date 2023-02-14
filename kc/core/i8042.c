#include "input.h"
#include "port.h"
#include "i8042.h"
#include "pic8259.h"
#include <lib/kstdio.h>

static void i8042_init(void);
static void i8042_enable(void) {}
static void i8042_disable(void) {}

static int i8042_append_callback(input_callback func) {(void)func; return 0;}
static void i8042_delete_callback(input_callback func) {(void)func;}

const struct input_source i8042_input_source =
{
    "i8042",
    i8042_init,
    i8042_enable,
    i8042_disable,

    i8042_append_callback,
    i8042_delete_callback,
};

#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define PS2_DATA 0x60
#define PS2_IRQ 1

static void i8042_disable_ports()
{
    outb(PS2_COMMAND, 0xad);
    outb(PS2_COMMAND, 0xa7);
}

static uint8_t i8042_command(int c)
{
    outb(PS2_COMMAND, c);
    return (uint8_t)c;
}

static uint8_t i8042_status()
{
    return inb(PS2_STATUS);
}

static bool i8042_read(uint8_t *b)
{
    if (i8042_status() & 1)
    {
        (b != NULL) ? *b = inb(PS2_DATA) : inb(PS2_DATA);
        return true;
    }

    return false;
}

static void i8042_flush()
{
    while(i8042_status() & 1)
    {
        i8042_read(NULL);
    }
}

static int i8042_callback(uint8_t irq)
{
    (void)irq;
    i8042_flush();
    return 0;
}


#define I8042_MAX_TRIES 4096

static bool i8042_write(uint8_t b)
{
    for (int i = 0; i < I8042_MAX_TRIES; i++)
    {
        if (i8042_status() & 2)
        {
            continue;
        }

        outb(PS2_DATA, b);
        return true;
    }

    return false;
}

static void i8042_set_config(void)
{
    bool channel_a_enable = false;
    bool channel_b_enable = false;

    bool maybe_dual_channel = false;
    bool is_dual_channel = false;
    i8042_command(0x20);
    uint8_t config_byte;
    i8042_read(&config_byte);
    kprintf("i8042: current config: %#0.2x\n", config_byte);
    config_byte &= 0xfc;
    if (config_byte & (0x1 << 5))
    {
        maybe_dual_channel = true;
    }
    config_byte |= 0x01;
    kprintf("i8042: new config: %#0.2x\n", config_byte);
    i8042_command(0x60);
    i8042_write(config_byte);
    i8042_command(0xaa);
    uint8_t bist_result;
    if (i8042_read(&bist_result) && bist_result != 0x55)
    {
        kprintf("i8042: warning: error self-testing device. aborting.\n");
        return;
    }
    
    if (maybe_dual_channel)
    {
        i8042_command(0xa8);
        i8042_command(0x20);
        uint8_t config_byte;
        if (i8042_read(&config_byte) && config_byte & (0x1 << 5))
        {
            kprintf("i8042: single-channel controller detected\n");
        }
        else
        {
            kprintf("i8042: dual-channel controller detected\n");
            is_dual_channel = true;
        }
    }
    
    outb(PS2_COMMAND, 0xab);
    if (inb(PS2_DATA) != 0x00)
    {
        kprintf("i8042: failed testing port a\n");
    }
    else
    {
        channel_a_enable = true;
    }
    if (is_dual_channel)
    {
        outb(PS2_COMMAND, 0xa9);
        if (inb(PS2_DATA) != 0x00)
        {
            kprintf("i8042: failed testing port b\n");
        }
        else
        {
            channel_b_enable = true;
        }
        i8042_disable_ports();
    }

    if (channel_a_enable)
    {
        outb(PS2_COMMAND, 0xae);
        i8042_write(0xff);
        if(inb(PS2_DATA) == 0xfa && inb(PS2_DATA) == 0xaa)
        {
            kprintf("i8042: detected device on channel, type %#0.2x\n", inb(PS2_DATA));
        }
    }

    i8042_flush();
    (void)channel_b_enable;
}

static void i8042_init(void)
{
    i8042_disable_ports();
    i8042_flush();
    i8042_set_config();

    pic8259_irq_install(PS2_IRQ, i8042_callback);
    pic8259_irq_unmask(PS2_IRQ);
}


