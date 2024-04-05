#include "shared.h"

int getpid()
{
	message msg = {.type = SYS_GET_PID};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.PID;
}
