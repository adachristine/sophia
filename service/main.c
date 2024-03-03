#include <unistd.h>
#include <stdio.h>

#define KJARNA_REVISION 1

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("kjarna %d\n", KJARNA_REVISION);

	while (1);

	return 0;
}

