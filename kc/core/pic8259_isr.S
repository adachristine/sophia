#include "cpu/interrupt.h"
#include "cpu/irq.h"
#include "pic8259.h"

IRQ_DECLARE pic8259, pic8259_irq_begin, 0, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 1, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 2, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 3, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 4, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 5, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 6, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 7, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 8, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 9, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 10, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 11, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 12, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 13, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 14, PIC8259_ISR_BASE
IRQ_DECLARE pic8259, pic8259_irq_begin, 15, PIC8259_ISR_BASE

IRQ_DEFINE pic8259, pic8259_irq_begin, 0
IRQ_DEFINE pic8259, pic8259_irq_begin, 1
IRQ_DEFINE pic8259, pic8259_irq_begin, 2
IRQ_DEFINE pic8259, pic8259_irq_begin, 3
IRQ_DEFINE pic8259, pic8259_irq_begin, 4
IRQ_DEFINE pic8259, pic8259_irq_begin, 5
IRQ_DEFINE pic8259, pic8259_irq_begin, 6
IRQ_DEFINE pic8259, pic8259_irq_begin, 7
IRQ_DEFINE pic8259, pic8259_irq_begin, 8
IRQ_DEFINE pic8259, pic8259_irq_begin, 9
IRQ_DEFINE pic8259, pic8259_irq_begin, 10
IRQ_DEFINE pic8259, pic8259_irq_begin, 11
IRQ_DEFINE pic8259, pic8259_irq_begin, 12
IRQ_DEFINE pic8259, pic8259_irq_begin, 13
IRQ_DEFINE pic8259, pic8259_irq_begin, 14
IRQ_DEFINE pic8259, pic8259_irq_begin, 15

