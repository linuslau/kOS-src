#include "shared.h"

int open(const char *pathname, int flags)
{
	message msg = {.type = FS_OPEN, .PATHNAME = (void*)pathname, .NAME_LEN = strlen(pathname), .FLAGS = flags};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.FD;
}
