#include "task.h"
#include "timer.h"
#include "memory.h"
#include "panic.h"
#include "cpu.h"
#include "kprint.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "pit8253.h"

#include <stdatomic.h>
#include <stdbool.h>

extern uint64_t *get_tss_rsp0(void);

static void lock_scheduler(void);
static void unlock_scheduler(void);
static void lock_preempt(void);
static void unlock_preempt(void);

static void update_time(void);

static struct kc_thread *create_thread(void (*thread_entry)(void));
static void destroy_thread(struct kc_thread *thread);
static void set_thread(struct kc_thread *thread);
static void block_thread(enum kc_thread_status reason);
static void unblock_thread(struct kc_thread *thread);
static void sleep_thread(uint64_t nanoseconds);
static void sleep_thread_until(uint64_t nanoseconds);

static int sleeping_thread_callback(uint64_t nanoseconds);

static struct kc_thread *ready_thread_pop(void);
static void ready_thread_push(struct kc_thread *thread);
static void ready_thread_push_back(struct kc_thread *thread);

const struct timer_source * timesource;

/* static threads that are always present
 * TODO: make this a per-CPU thing at some point
 */
static struct kc_thread *idle_thread;

// thread lists for the scheduler to manipulate
static struct kc_thread *current_thread;
static struct kc_thread *first_ready_thread;
static struct kc_thread *last_ready_thread;
static struct kc_thread *sleeping_threads;

static volatile atomic_uint_fast64_t preempt_switch_count = 0;
static volatile atomic_bool preempt_switch_flag = false;

static void idle_thread_entry(void)
{
    kprintf("idle thread started\n");
    unlock_scheduler();

    while (true)
    {
        __asm__ volatile ("hlt;");
    }
}

noreturn void task_init(void)
{
    lock_scheduler();
    timesource = &pit8253_timer_source;
    timesource->append_callback(sleeping_thread_callback);
    timesource->start();
    kprintf(
            "starting task management, timesource delta %luns\n",
            timesource->nanoseconds_delta());

    // XXX: unfuck this mess at some point
    // XXX: make this also not demand-allocated or it's gonna fail
    char *kernel_rsp0 = vm_alloc(4096, VM_ALLOC_ANY);
    kernel_rsp0[1] = 0;
    *get_tss_rsp0() = (uintptr_t)kernel_rsp0 + 4096;

    // initalize static threads
    idle_thread = create_thread(idle_thread_entry);

    current_thread = idle_thread;

    idle_thread->status = RUNNING;
    cpu_set_thread(&idle_thread->state, NULL, get_tss_rsp0());

    // shouldn't ever get here
    PANIC(DEAD_END);
}

void task_schedule(void)
{
    if (atomic_load(&preempt_switch_count))
    {
        preempt_switch_flag = true;
        return;
    }

    if (first_ready_thread)
    {
        struct kc_thread *this_thread = ready_thread_pop();

        if (this_thread == idle_thread)
        {
            if (first_ready_thread)
            {
                this_thread->status = READY;
                this_thread = ready_thread_pop();
                ready_thread_push(idle_thread);
            }
            else if (current_thread->status == RUNNING)
            {
                return;
            }
            else
            {
                // NULL statement????? idon't like
            }
        }
        set_thread(this_thread);
    }
}

static void lock_scheduler(void)
{
    irq_lock();
}

static void lock_preempt(void)
{
    irq_lock();
    preempt_switch_count++;
}

static void unlock_scheduler(void)
{
    irq_unlock();
}

static void unlock_preempt(void)
{
    if (atomic_load(&preempt_switch_count) >= 1)
    {
        preempt_switch_count--;
    }

    if (!atomic_load(&preempt_switch_count) && 
            atomic_load(&preempt_switch_flag))
    {
        preempt_switch_flag = false;
        task_schedule();
    }

    irq_unlock();
}

void update_time(void)
{
    static uint64_t last_elapsed = 0;
    if (current_thread)
    {
        uint64_t current_elapsed = timesource->nanoseconds_elapsed();
        uint64_t delta = current_elapsed - last_elapsed;
        last_elapsed = current_elapsed;
        current_thread->time_elapsed += delta;
    }
}

static struct kc_thread *create_thread(void (*thread_f)(void))
{
    char *task_bottom = vm_alloc(16384, VM_ALLOC_ANY);
    struct kc_thread *thread =
        (struct kc_thread *)(task_bottom + 16384 - sizeof(*thread));

    thread->next = NULL;

    thread->time_elapsed = 0;
    thread->sleep_expiration = 0;

    thread->state.stack = (uintptr_t)thread;
    thread->state.stack_top = *get_tss_rsp0();
    thread->state.page_map = mmu_get_map();

    // set up the expected stack values for the state
    struct task_register_state
    {
        uint64_t rbp;
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
        uint64_t rip;
    }
    *register_state =
        (struct task_register_state *)
        (thread->state.stack -= sizeof(*register_state));

    memset(register_state, 0, sizeof(*register_state));

    register_state->rip = (uint64_t)thread_f;
    register_state->rbp = thread->state.stack;

    thread->status = READY;

    return thread;
}

static void destroy_thread(struct kc_thread *thread)
{
    (void)thread;
}

static void set_thread(struct kc_thread *thread)
{
    if (atomic_load(&preempt_switch_count))
    {
        preempt_switch_flag = true;
        return;
    }
    update_time();
    struct kc_thread *previous_thread = current_thread;
    current_thread = thread;
    
    if (previous_thread->status == RUNNING)
    {
        previous_thread->status = READY;
        ready_thread_push(previous_thread);
    }

    cpu_set_thread(
            &current_thread->state,
            &previous_thread->state,
            get_tss_rsp0());

    current_thread->status = RUNNING;
}

static void block_thread(enum kc_thread_status reason)
{
    lock_scheduler();
    current_thread->status = reason;
    task_schedule();
    unlock_scheduler();
}

static void unblock_thread(struct kc_thread *thread)
{
    lock_scheduler();

    thread->status = READY;

    if (!first_ready_thread || (current_thread == idle_thread))
    {
        unlock_preempt();
        set_thread(thread);
    }
    else
    {
        ready_thread_push_back(thread);
    }

    unlock_scheduler();
}

static void sleep_thread(uint64_t nanoseconds)
{
    sleep_thread_until(timesource->nanoseconds_elapsed() + nanoseconds);
}

static void sleep_thread_until(uint64_t nanoseconds)
{
    lock_preempt();

    if (nanoseconds < timesource->nanoseconds_elapsed())
    {
        unlock_scheduler();
        return;
    }

    kprintf("time elapsed is now %ld\r\n", timesource->nanoseconds_elapsed());
    kprintf("thread will now sleep until %ld\r\n", nanoseconds);
    current_thread->sleep_expiration = nanoseconds;
    current_thread->next = sleeping_threads;
    sleeping_threads = current_thread;

    unlock_preempt();

    block_thread(SLEEPING);
}

static struct kc_thread *ready_thread_pop(void)
{
    struct kc_thread *thread = first_ready_thread;

    if (thread)
    {
        first_ready_thread = thread->next;
    }

    if (!first_ready_thread)
    {
        last_ready_thread = NULL;
    }

    return thread;
}

static void ready_thread_push(struct kc_thread *thread)
{
    thread->next = first_ready_thread;
    first_ready_thread = thread;

    if (!last_ready_thread)
    {
        last_ready_thread = first_ready_thread;
    }
}

static void ready_thread_push_back(struct kc_thread *thread)
{
    if (last_ready_thread)
    {
        last_ready_thread->next = thread;
    }
    else
    {
        first_ready_thread = thread;
    }
    last_ready_thread = thread;
}

static int sleeping_thread_callback(uint64_t nanoseconds)
{
    lock_preempt();

    struct kc_thread *sleeping = sleeping_threads;
    sleeping_threads = NULL;

    while (sleeping != NULL)
    {
        struct kc_thread *this_thread = sleeping;
        sleeping = sleeping->next;

        if (this_thread->sleep_expiration <= nanoseconds)
        {
            kprintf("thread awakened: %p\n", this_thread);
            this_thread->sleep_expiration = 0;
            unblock_thread(this_thread);
        }
        else
        {
            this_thread->next = sleeping_threads;
            sleeping_threads = this_thread;
        }
    }

    unlock_preempt();

    lock_scheduler();
    task_schedule();
    unlock_scheduler();

    return 0;
}

