#include "task.h"
#include "timer.h"
#include "memory.h"
#include "panic.h"
#include "cpu.h"
#include "kprint.h"
#include "cpu/irq.h"

#include "pit8253.h"

#include <stdatomic.h>
#include <stdbool.h>

struct kc_thread *current_task = NULL;
struct kc_thread *first_ready_task = NULL;
struct kc_thread *last_ready_task = NULL;
struct kc_thread *sleeping_tasks = NULL;
struct kc_thread *blocked_tasks = NULL;

static void lock_scheduler(void);
static void unlock_scheduler(void);
static void set_thread_status(enum kc_thread_status status);
static void idle_task_thread(void);

extern uint64_t *get_tss_rsp0(void);
static uint64_t last_count = 0;

const struct timer_source * timesource;

static atomic_int preempt_switch_count = 0;
static volatile atomic_bool preempt_switch_flag = false;

void update_time_used(void)
{
    uint64_t current_count = timesource->nanoseconds_elapsed();
    uint64_t elapsed = current_count - last_count;
    last_count = current_count;
    current_task->time_elapsed += elapsed;
}

void task_set_thread(struct kc_thread *task)
{
    // wrapper around cpu_set_thread
    if (preempt_switch_count)
    {
        preempt_switch_flag = true;
        return;
    }
    uint64_t *rsp0 = get_tss_rsp0();
    cpu_set_thread(&current_task->state, &task->state, rsp0);
}

void task_schedule(void)
{
    if (preempt_switch_count)
    {
        preempt_switch_flag = true;
        return;
    }

    update_time_used();
    if (first_ready_task)
    {
        struct kc_thread *task = first_ready_task;
        first_ready_task = task->next;
        task_set_thread(task);
    }
}


static void lock_scheduler(void)
{
    irq_lock();
    preempt_switch_count++;
}

static void unlock_scheduler(void)
{
    if (preempt_switch_count >= 1)
    {
        preempt_switch_count--;
    }

    if (!preempt_switch_count && preempt_switch_flag)
    {
        preempt_switch_flag = false;
        task_schedule();
    }

    irq_unlock();
}

static void unblock_thread(struct kc_thread *thread)
{
    lock_scheduler();
    if (!first_ready_task)
    {
        task_set_thread(thread);
    }
    else
    {
        first_ready_task->next = thread;
        first_ready_task = thread;
    }
    unlock_scheduler();
}

static void set_thread_status(enum kc_thread_status status)
{
    lock_scheduler();
    current_task->status = status;
    task_schedule();
    unlock_scheduler();
}

static void idle_task_thread(void)
{
    kputs("in idle thread\n");
    while (1)
    {
        __asm__ volatile ("hlt;");
        lock_scheduler();
        task_schedule();
        unlock_scheduler();
    }
}

noreturn void task_init(void)
{
    lock_scheduler();
    timesource = &pit8253_timer_source;
    timesource->start();
    char *kernel_rsp0 = vm_alloc(4096);
    // TODO: make it so this isn't demand-allocated.
    // TODO: decouple the task management code from cpu-dependent task code
    kernel_rsp0[1] = 0;
    *get_tss_rsp0() = (uintptr_t)kernel_rsp0;
    char *first_task = vm_alloc(16384);
    struct kc_thread *idle_thread =
        (struct kc_thread *)(first_task + 16384 - sizeof(*idle_thread));
    idle_thread->state.stack = (uintptr_t)idle_thread;
    idle_thread->state.stack_top = (uintptr_t)kernel_rsp0;

    // XXX: this is kindof a train wreck
    __asm__ volatile
        (
         "mov %%cr3, %%rax\n\t"
         "mov %%rax, 0(%%rsi)\n\t"
         "mov %%rcx, %%rsp\n\t"
         "call unlock_scheduler\n\t"
         "jmp idle_task_thread\n\t"
         : "=m"(idle_thread->state.page_map)
         : "S"(&idle_thread->state.page_map),
         "c"(idle_thread)
         : "rax"
        );

    // shouldn't ever get here
    PANIC(DEAD_END);
}

