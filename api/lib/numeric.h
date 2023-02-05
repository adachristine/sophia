#pragma once
#pragma GCC diagnostic ignored "-Wgnu-statement-expression"

#define min(x, y) \
    ({ \
     __typeof__ (x) _x = (x); \
     __typeof__ (y) _y = (y); \
     _x < _y ? _x : _y; \
     })

#define max(x, y) \
    ({ \
     __typeof__ (x) _x = (x); \
     __typeof__ (y) _y = (y); \
     _x > _y ? _x : _y; \
     })

