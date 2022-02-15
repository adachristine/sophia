#pragma once

#include "interrupt.h"

#ifndef __ASSEMBLER__

void irq_lock(void);
void irq_unlock(void);

#else

.macro IRQ_DECLARE name:req handler:req, irq:req, offset:req
.hidden \name\()_irq\()\irq\()_isr
.global \name\()_irq\()\irq\()_isr
.extern \handler
.type \name\()_irq\()\irq\()_isr, @function
.endm

.macro IRQ_DEFINE name:req handler:req, irq:req
\name\()_irq\()\irq\()_isr:
    ISR_STORE_PARAM_REGS
    movb \irq, %dil
    ISR_STORE_CALLER_REGS
    ISR_STORE_CALLEE_REGS
    ISR_CALL \handler
    ISR_RESTORE_CALLEE_REGS
    ISR_RESTORE_CALLER_REGS
    ISR_RESTORE_PARAM_REGS
    iretq
.size \name\()_irq\()\irq\()_isr, . - \name\()_irq\()\irq\()_isr
.endm

#endif

