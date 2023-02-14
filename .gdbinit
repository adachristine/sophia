target remote localhost:1234
add-symbol-file kc/core/kernel.os -o 0xffffffff80000000
break i8042_callback

