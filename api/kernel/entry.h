#pragma once

#include <stddef.h>
#include <kernel/memory/range.h>

enum video_pixel_format
{
    NO_VIDEO_FORMAT,
    RGBA32BPP_VIDEO_FORMAT,
    BGRA32BPP_VIDEO_FORMAT
};

enum acpi_version
{
    ACPI_LEGACY,
    ACPI_CURRENT
};

struct kc_boot_data_buffer
{
    unsigned char *base;
    unsigned char *current;
    size_t max_size;
    size_t available_size;
};

struct kc_boot_memory_data
{
    struct memory_range *entries;
    size_t count;
};

struct kc_boot_video_data
{
    struct memory_range framebuffer;
    uint16_t width;
    uint16_t height;
    size_t pitch;
    enum video_pixel_format format;
};

struct kc_boot_acpi_data
{
    void *rsdp;
    enum acpi_version version;
};

typedef int (*kc_entry_func)(void *parameters);

struct kc_boot_data
{
    struct kc_boot_data_buffer buffer;
    struct kc_boot_memory_data memory;
    struct kc_boot_video_data video;
    struct kc_boot_acpi_data acpi;
};

struct kc_boot_data *get_boot_data(void);

