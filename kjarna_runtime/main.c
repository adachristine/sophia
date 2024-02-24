#include <kjarna/interface.h>

extern int write(int, void *, size_t);

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	write(1, "hello, runtime world!\r\n", -1);

	while (1);

	return 0;
}

struct kjarna_entry_params *entry_params;

int kjarna_entry(struct kjarna_entry_params *params)
{
	entry_params = params;
	entry_params->interface->write(1, "hello, entry world\r\n", -1);

	return main(params->argc, params->argv);
}

