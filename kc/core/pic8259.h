#pragma once

#define PIC_PRI_IOBASE 0x20
#define PIC_SEC_IOBASE 0xA0
#define PIC_PRI_CMD PIC_PRI_IOBASE
#define PIC_SEC_CMD PIC_SEC_IOBASE
#define PIC_PRI_DATA (PIC_PRI_IOBASE+1)
#define PIC_SEC_DATA (PIC_SEC_IOBASE+1)

#define PIC_ISR_BASE 240
#define PIC_ISR_COUNT 16

#ifndef __ASSEMBLER__

#include <stdint.h>

void pic8259_init(void);
void pic8259_irq_begin(uint8_t irq);
void pic8259_irq_end(uint8_t irq);
void pic8259_irq_mask(uint8_t irq);
void pic8259_irq_unmask(uint8_t irq);

extern void pic8259_irq0_isr(void);
extern void pic8259_irq1_isr(void);
extern void pic8259_irq2_isr(void);
extern void pic8259_irq3_isr(void);
extern void pic8259_irq4_isr(void);
extern void pic8259_irq5_isr(void);
extern void pic8259_irq6_isr(void);
extern void pic8259_irq7_isr(void);
extern void pic8259_irq8_isr(void);
extern void pic8259_irq9_isr(void);
extern void pic8259_irq10_isr(void);
extern void pic8259_irq11_isr(void);
extern void pic8259_irq12_isr(void);
extern void pic8259_irq13_isr(void);
extern void pic8259_irq14_isr(void);
extern void pic8259_irq15_isr(void);

#endif

