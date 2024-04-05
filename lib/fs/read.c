#include "shared.h"

int read(int fd, void *buf, int count)
{
	message msg = {.type = FS_READ, .FD = fd, .BUF = buf, .CNT = count};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);

	return msg.CNT;
}
