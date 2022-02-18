#pragma once

#include <stdint.h>

enum kc_thread_status
{
    READY,
    RUNNING,
    SLEEPING,
    BLOCKED,
    TERMINATED
};

struct kc_thread_state
{
    uint64_t stack;
    uint64_t stack_top;
    uint64_t page_map;
};

struct kc_thread
{
    struct kc_thread *next;
    uint64_t time_elapsed;
    uint64_t sleep_expiration;
    enum kc_thread_status status;
    struct kc_thread_state state;
};

extern void cpu_set_thread(
        struct kc_thread_state *current,
        struct kc_thread_state *next,
        uint64_t *cpu_tss_rsp0);

void task_init(void);
void task_schedule(void);

