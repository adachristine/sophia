#include <unistd.h>

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	write(1, "hello, runtime world!\r\n", -1);

	while (1);

	return 0;
}

