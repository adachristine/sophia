# vim: ft=make:nowrap:ts=4:sw=4
OBJCOPY.info = $(info $$(OBJCOPY) $< $@)

LINK.os = $(LD) $(LDFLAGS) $(LIBDIRS) -T $(LDSCRIPT) $(filter %.o, $^) $(LOADLIBES) -o $@

LINK.info = $(info $$(LD) $@)

COMPILE.c = $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

COMPILE.S = $(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@

COMPILE.cpp = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

COMPILE.info = $(info $$(CC) $@)

%.os:
	$(LINK.info)
	$(LINK.os)

%.debug.os: %.os
	$(OBJCOPY) --only-keep-debug $< $@
	@$(OBJCOPY) --strip-debug $<

%.o: %.c
	$(COMPILE.info)
	$(COMPILE.c) -MMD -MP

%.o: %.cpp
	$(COMPILE.info)
	$(COMPILE.cpp) -MMD -MP

%.o: %.S
	$(COMPILE.info)
	$(COMPILE.S) -MMD -MP

clean:
	$(info $$(RM) $(CLEANLIST))
	-@$(RM) $(CLEANLIST)

distclean: clean
	$(info $$(RM) $(DCLEANLIST))
	-@$(RM) $(DCLEANLIST)

.PHONY: clean distclean all

