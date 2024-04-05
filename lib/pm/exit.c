#include "shared.h"

void exit(int status)
{
	message msg;
	msg.type	= EXIT;
	msg.STATUS	= status;

	ipc_msg_send_rcv(SEND_RECEIVE, TASK_PM, &msg);
	assert(msg.type == SYSCALL_RET);
}
