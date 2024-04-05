#include "shared.h"

int write(int fd, const void *buf, int count)
{
	message msg;
	msg.type = FS_WRITE;
	msg.FD   = fd;
	msg.BUF  = (void*)buf;
	msg.CNT  = count;

	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);

	return msg.CNT;
}
