#include "shared.h"

int stat(const char *path, struct stat *buf)
{
	message msg = {.type = FS_STAT, .PATHNAME = (void*)path, .BUF = (void*)buf, .NAME_LEN = strlen(path)};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}
