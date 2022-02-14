#include "pic8259.h"
#include "cpu.h"
#include "port.h"
#include "kprint.h"

#include <stdbool.h>

#define ICW1_ICW4 0x1
#define ICW1_SINGLE 0x2
#define ICW1_INTERVAL4 0x4
#define ICW1_LEVEL 0x8
#define ICW1_INIT 0x10

#define ICW4_8086 0x1
#define ICW4_AUTO 0x2
#define ICW4_BUF_SEC 0x8
#define ICW4_BUF_PRI 0xc
#define ICW4_SFNM 0x10

#define OCW3_READ_IRR 0xa
#define OCW3_READ_ISR 0xb
#define OCW3_EOI 0x20

pic8259_handler_func irq_handlers[PIC_ISR_COUNT];

void pic8259_irq_install(uint8_t irq, pic8259_handler_func h)
{
    if (irq_handlers[irq])
    {
        kprintf("attempt to overwrite irq handler for %u\n", irq);
        return;
    }
    irq_handlers[irq] = h;
}

static void wait(void)
{
    // stall for a few cycles
    inb(0x80);
    inb(0x80);
}

void pic8259_init(void)
{
    // basic init. not gonna fret with this too much
    outb(PIC_PRI_CMD, ICW1_INIT|ICW1_ICW4);
    wait();
    outb(PIC_SEC_CMD, ICW1_INIT|ICW1_ICW4);
    wait();
    outb(PIC_PRI_DATA, PIC_ISR_BASE);
    wait();
    outb(PIC_SEC_DATA, PIC_ISR_BASE+8);
    wait();
    outb(PIC_PRI_DATA, 4);
    wait();
    outb(PIC_SEC_DATA, 2);
    wait();
    outb(PIC_PRI_DATA, ICW4_8086);
    wait();
    outb(PIC_SEC_DATA, ICW4_8086);
    wait();
    // mask all interrupt lines
    outb(PIC_PRI_DATA, 0xff);
    outb(PIC_SEC_DATA, 0xff);

    isr_install(PIC_ISR_BASE + 0, pic8259_irq0_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 1, pic8259_irq1_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 2, pic8259_irq2_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 3, pic8259_irq3_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 4, pic8259_irq4_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 5, pic8259_irq5_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 6, pic8259_irq6_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 7, pic8259_irq7_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 8, pic8259_irq8_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 9, pic8259_irq9_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 10, pic8259_irq10_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 11, pic8259_irq11_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 12, pic8259_irq12_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 13, pic8259_irq13_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 14, pic8259_irq14_isr, 0, 0);
    isr_install(PIC_ISR_BASE + 15, pic8259_irq15_isr, 0, 0);
}

static bool irq_is_spurious(uint8_t irq)
{
    uint8_t isr_pri;
    uint8_t isr_sec;

    // all other no other IRQs are 
    if (irq != 7 || irq != 15)
    {
        return false;
    }
    else if (irq == 7)
    {
        outb(PIC_PRI_CMD, OCW3_READ_ISR);
        isr_pri = inb(PIC_PRI_CMD);

        if (isr_pri == 0x80)
        {
            return false;
        }
    }
    else if (irq == 15)
    {
        outb(PIC_SEC_CMD, OCW3_READ_ISR);
        isr_sec = inb(PIC_SEC_CMD);

        if (isr_sec == 0x80)
        {
            return false;
        }
        else
        {
            // tell primary to EOI
            outb(PIC_PRI_CMD, OCW3_EOI);
        }
    }

    return true;
}

void pic8259_irq_mask(uint8_t irq)
{
    uint16_t port = PIC_PRI_DATA;
    uint8_t mask;

    if (irq > 7)
    {
        irq -= 8;
        port = PIC_SEC_DATA;
        
    }

    mask = inb(port);
    mask |= (1 << irq);
    outb(port, mask);
        
    // mask the primary irq2 pin if there are no active lines
    // on the secondary
    if (irq > 7 && mask == 0xff)
    {
        pic8259_irq_mask(2);
    }
}

void pic8259_irq_unmask(uint8_t irq)
{
    uint8_t mask;
    uint16_t port = PIC_PRI_DATA;

    if (irq > 7)
    {
        irq -= 8;
        port = PIC_SEC_DATA;
        // unmask the primary's cascade line
        pic8259_irq_unmask(2);
    }

    mask = inb(port);
    mask &= ~(1 << irq);
    outb(port, mask);
}

void pic8259_irq_begin(uint8_t irq)
{
    int result;

    // check for spurious activations
    if (irq_is_spurious(irq))
    {
        kprintf("spurious activation of irq%hhu\n", irq);
        return;
    }

    else if (irq_handlers[irq])
    {
        result = irq_handlers[irq](irq);

        if (result)
        {
            kprintf("error handling irq%hhu: % d\n", irq, result);
        }
    }
    else
    {
        kprintf("%s of irq%hhu\n", "unhandled activation", irq);
    }

    pic8259_irq_end(irq);
}

void pic8259_irq_end(uint8_t irq)
{
    if (irq > 7)
    {
        outb(PIC_SEC_CMD, OCW3_EOI);
    }

    outb(PIC_PRI_CMD, OCW3_EOI);
}

