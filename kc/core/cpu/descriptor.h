#pragma once

struct dtr64
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct segment_descriptor
{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base16;
    uint8_t access;
    uint8_t flags_limit16;
    uint8_t base24;
} __attribute__((packed));

enum segment_descriptor_type
{
    CODE64_SUPER_SEG,
    CODE64_USER_SEG,
    DATA_SUPER_SEG,
    TASK64_SEG,
    CODE32_USER_SEG,
    DATA_USER_SEG
};

struct gate_descriptor
{
    uint16_t offset0;
    uint16_t selector;
    uint8_t ist;
    uint8_t access;
    uint16_t offset16;
    uint64_t offset32;
} __attribute__((packed));

enum gate_descriptor_type
{
    EMPTY_GATE,
    INT64_GATE,
    TRAP64_GATE,
};

struct task64_segment
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint16_t reserved2;
    uint16_t iomap_base;
};

