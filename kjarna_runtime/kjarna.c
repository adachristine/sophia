#include <kjarna/interface.h>

struct kjarna_entry_params *entry_params;

extern int main(int argc, char **argv);

int kjarna_entry(struct kjarna_entry_params *params)
{
	entry_params = params;

	return main(params->argc, params->argv);
}

