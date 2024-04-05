#include "shared.h"

int ipc_msg_send_rcv(int function, int src_dest, message* msg)
{
	int ret = 0;

	if (function == RECEIVE)
		memset(msg, 0, sizeof(message));

	switch (function) {
		case SEND_RECEIVE:
			ret = sys_send_rcv(SEND, src_dest, msg);
			if (ret == 0)
				ret = sys_send_rcv(RECEIVE, src_dest, msg);
			break;
		case SEND:
		case RECEIVE:
			ret = sys_send_rcv(function, src_dest, msg);
			break;
		default:
			assert((function == SEND_RECEIVE) ||
				(function == SEND) || (function == RECEIVE));
			break;
	}

	return ret;
}