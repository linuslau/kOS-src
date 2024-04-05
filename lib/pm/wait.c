#include "shared.h"

int wait(int * status)
{
	message msg;
	msg.type   = WAIT;

	ipc_msg_send_rcv(SEND_RECEIVE, TASK_PM, &msg);

	*status = msg.STATUS;

	return (msg.PID == NO_TASK ? -1 : msg.PID);
}
