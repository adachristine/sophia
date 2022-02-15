#include "task.h"
#include "timer.h"
#include "memory.h"
#include "panic.h"
#include "cpu.h"
#include "kprint.h"
#include "cpu/irq.h"

struct kc_thread *current_task = NULL;
struct kc_thread *first_ready_task = NULL;
struct kc_thread *last_ready_task = NULL;

static void lock_scheduler(void);
static void unlock_scheduler(void);
static void set_thread_status(enum kc_thread_status status);
static void idle_task_thread(void);

extern uint64_t *get_tss_rsp0(void);
static uint64_t last_count = 0;

void update_time_used(void)
{
    uint64_t current_count = timer_ticks();
    uint64_t elapsed = current_count - last_count;
    last_count = current_count;
    current_task->time_elapsed += elapsed;
}

void task_set_thread(struct kc_thread *task)
{
    // wrapper around cpu_set_thread
    uint64_t *rsp0 = get_tss_rsp0();
    cpu_set_thread(&current_task->state, &task->state, rsp0);
}

void task_schedule(void)
{
    kprintf("task_schedule() called @%lu ticks\n", last_count);
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
}

static void unlock_scheduler(void)
{
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
    char *kernel_rsp0 = vm_alloc(4096);
    // TODO: make it so this isn't demand-allocated.
    kernel_rsp0[1] = 0;
    *get_tss_rsp0() = (uintptr_t)kernel_rsp0;
    char *first_task = vm_alloc(16384);
    struct kc_thread *idle_thread =
        (struct kc_thread *)(first_task + 16384 - sizeof(*idle_thread));
    idle_thread->state.stack = (uintptr_t)idle_thread;
    idle_thread->state.stack_top = (uintptr_t)kernel_rsp0;

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
    panic(GENERAL_PANIC);
}

