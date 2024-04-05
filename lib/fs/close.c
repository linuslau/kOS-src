#include "shared.h"

int close(int fd)
{
	message msg = {.type = FS_CLOSE, .FD = fd};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);

	return msg.RETVAL;
}
