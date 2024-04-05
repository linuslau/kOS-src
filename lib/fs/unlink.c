#include "shared.h"

/* Remove/delete a file*/
int unlink(const char * pathname)
{
	message msg = {.type = FS_UNLINK, .PATHNAME = (void*)pathname, .NAME_LEN = strlen(pathname)};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg);
	return msg.RETVAL;
}
