#pragma once

typedef int (*input_callback)(void);

struct input_callback_list
{
    input_callback func;
    struct input_callback_list *next;
};

struct input_source
{
    const char *name;
    void (*init)(void);
    void (*enable)(void);
    void (*disable)(void);

    int (*append_callback)(input_callback func);
    void (*delete_callback)(input_callback func);
};

