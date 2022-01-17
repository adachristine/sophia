#pragma once

#include <stdint.h>

#define EFER_SCE 1

#define MSR_EFER 0xc0000080
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_SFMASK 0xc0000084

uint64_t msr_read(uint32_t index);
void msr_write(uint32_t index, uint64_t value);

