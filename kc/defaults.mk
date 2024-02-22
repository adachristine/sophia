

CFLAGS += -fPIC -fvisibility=hidden -mgeneral-regs-only

LDFLAGS += -z max-page-size=4096 --export-dynamic -pic \
	   -z separate-code -pie -Bsymbolic -shared \
	   -z noexecstack -g

