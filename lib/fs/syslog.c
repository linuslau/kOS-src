#include "shared.h"

int syslog(const char *fmt, ...)
{
	char buf[STR_BUFFER_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /* 4: size of `fmt' in the stack */
	int i = vsprintf(buf, fmt, arg);
	assert(strlen(buf) == i);

	return disklog(buf);
}

