#include "shared.h"

int printf(const char *fmt, ...)
{
	int write_len, written_len;
	char buf[STR_BUFFER_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /** 4: size of `fmt' in the stack */
	write_len 	= vsprintf(buf, fmt, arg);
	written_len = write(1, buf, write_len);

	assert(write_len == written_len);

	return write_len;
}

/* low level print */
int printl(const char *fmt, ...)
{
	int write_len;
	char buf[STR_BUFFER_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /** 4: size of `fmt' in the stack */

	write_len 	= vsprintf(buf, fmt, arg);
	message msg;
	msg.type 	= PRINTX;
	msg.BUF  	= (void*)buf;

	ipc_msg_send_rcv(SEND, TASK_TTY, &msg);
	return write_len;
}

