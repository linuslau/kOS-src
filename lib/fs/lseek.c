#include "shared.h"

int lseek(int fd, int offset, int whence)
{
	message msg = {.type = FS_LSEEK, .FD = fd, .OFFSET = offset, .WHENCE = whence};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);

	return msg.OFFSET;
}
