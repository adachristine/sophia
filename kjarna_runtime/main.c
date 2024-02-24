#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	fputs("hello, runtime world!\n", stdout);

	while (1);

	return 0;
}

