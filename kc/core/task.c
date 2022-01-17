#include "task.h"
#include "memory.h"
#include "panic.h"
#include "cpu.h"

struct kc_thread *current_task = NULL;
struct kc_thread *first_ready_task = NULL;
struct kc_thread *last_ready_task = NULL;

static int schedule_lock_cnt = 0;

static void lock_scheduler(void);
static void unlock_scheduler(void);
static void set_thread_status(enum kc_thread_status status);
static void idle_task_thread(void);

extern uint64_t *get_tss_rsp0(void);

void task_set_thread(struct kc_thread *task)
{
    // wrapper around cpu_set_thread
    uint64_t *rsp0 = get_tss_rsp0();
    cpu_set_thread(&current_task->state, &task->state, rsp0);
}

void task_schedule(void)
{
    if (first_ready_task)
    {
        struct kc_thread *task = first_ready_task;
        first_ready_task = task->next;
        task_set_thread(task);
    }
}

static void lock_scheduler(void)
{
    __asm__ volatile ("cli;");
    schedule_lock_cnt++;
}

static void unlock_scheduler(void)
{
    if (1 == schedule_lock_cnt)
    {
        schedule_lock_cnt--;
        __asm__ volatile("sti;");
    }
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
}

