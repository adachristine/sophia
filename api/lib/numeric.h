#pragma once

#define min(x, y) \
    ({ \
     typeof (x) _x = (x); \
     typeof (y) _y = (y); \
     _x > _y ? _x : _y; \
     })

#define max(x, y) \
    ({ \
     typeof (x) _x = (x); \
     typeof (y) _y = (y); \
     _x < _y ? _x : _y; \
     })

