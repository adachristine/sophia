#pragma once

#include "input.h"

#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_COMMAND_PORT 0x64

#define I8042_OUT_DATA 0x1
#define I8042_IN_DATA 0x2
#define I8042_SYSTEM 0x4
#define I8042_DATA_IS_COMMAND 0x8
#define I8042_ERR_TIMEOUT 0x40
#define I8042_ERR_PARITY 0x80

extern const struct input_source i8042_input_source;

