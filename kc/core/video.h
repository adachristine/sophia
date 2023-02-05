#pragma once

#include <stdint.h>

int video_init(void);
int video_putchar(int c);

struct video_point
{
    int32_t x;
    int32_t y;
};

struct video_size
{
    int32_t width;
    int32_t height;
};

struct video_rect
{
    struct video_point pos;
    struct video_size size;
};

struct video_color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

