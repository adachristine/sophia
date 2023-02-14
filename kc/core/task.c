#include "task.h"
#include "timer.h"
#include "memory.h"
#include "panic.h"
#include "cpu.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "pit8253.h"
#include "port.h"

#include <stdatomic.h>
#include <stdbool.h>

#include <lib/elf.h>
#include <lib/kstdio.h>

static const size_t INTERRUPT_STACK_SIZE = 4096;
static const size_t THREAD_SIZE = 16834;
static const enum vm_alloc_flags ALLOC_FLAGS = VM_ALLOC_ANY|VM_ALLOC_ANONYMOUS;

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
    
    __asm__ ("sti");

    while (true)
    {
        __asm__ ("hlt;");
    }
}

#define PS2INPUT 0x60
#define PS2STATUS 0x64
#define PS2STATUS_IBF 0x1

static void keyboard_input_loop(void)
{
    bool has_bytes = true;
    while (has_bytes)
    {
        has_bytes = inb(PS2STATUS) & PS2STATUS_IBF;
        if (has_bytes)
        {
            kprintf("read byte from keyboard input: %hhd\n", inb(PS2INPUT));
        }
        else
        {
            kprintf("no bytes in keyboard input\n");
        }
    }
}

static void sleepy_thread_entry(void)
{
    int thread_slept_count = 0;

    while (true)
    {
        sleep_thread(1000000000);
        thread_slept_count++;
        //keyboard_input_loop();
    }

    (void)thread_slept_count;
}

static void create_interrupt_stack(size_t size)
{
    char *rsp0 = vm_alloc(size, ALLOC_FLAGS);
    memset(rsp0, 0, size);
    *get_tss_rsp0() = (uintptr_t)rsp0 + size;
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
    create_interrupt_stack(INTERRUPT_STACK_SIZE);

    // initalize static threads
    idle_thread = create_thread(idle_thread_entry);
    struct kc_thread *sleepy_thread = create_thread(sleepy_thread_entry);

    current_thread = idle_thread;
    ready_thread_push_back(sleepy_thread);

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

static uint64_t scheduler_flags;

static void lock_scheduler(void)
{
    scheduler_flags = irq_lock();
}

static void lock_preempt(void)
{
    scheduler_flags = irq_lock();
    preempt_switch_count++;
}

static void unlock_scheduler(void)
{
    irq_unlock(scheduler_flags);
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

    irq_unlock(scheduler_flags);
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
    char *task_bottom = vm_alloc(16384, ALLOC_FLAGS);
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

//    kprintf("time elapsed is now %ld\r\n", timesource->nanoseconds_elapsed());
//    kprintf("thread will now sleep until %ld\r\n", nanoseconds);
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
            //kprintf("thread awakened: %p\n", this_thread);
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

