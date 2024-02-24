#include <libc/stdio.h>
#include <libc/string.h>
#include <posix/unistd.h>

struct FILE
{
	int fd;
};

static FILE stdin_stream = { STDIN_FILENO };
static FILE stdout_stream = { STDOUT_FILENO };
static FILE stderr_stream = { STDERR_FILENO };

FILE *stdin = &stdin_stream;
FILE *stdout = &stdout_stream;
FILE *stderr = &stderr_stream;

int fputc(int c, FILE *f)
{
	write(f->fd, (const char *)&c, 1);
	return 1;
}

int fputs(const char *s, FILE *f)
{
	size_t slen = strlen(s);
	write(f->fd, s, slen);
	return (int)slen;
}

