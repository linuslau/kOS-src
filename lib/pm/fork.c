#include "shared.h"

int fork()
{
	message msg;
	msg.type = FORK;

	ipc_msg_send_rcv(SEND_RECEIVE, TASK_PM, &msg);
	assert(msg.type == SYSCALL_RET);
	assert(msg.RETVAL == 0);

	return msg.PID;
}
