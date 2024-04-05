#include "shared.h"
#include "hd.h"

void task_hd()
{
	message msg;
	hd_init();

	while (1) {
		ipc_msg_send_rcv(RECEIVE, ANY, &msg);
		int pid = msg.source;

		switch (msg.type) {
			case DEV_OPEN:
				hd_open(msg.NR_DEV_MINOR);
				break;

			case DEV_CLOSE:
				hd_close(msg.NR_DEV_MINOR);
				break;

			case DEV_READ:
			case DEV_WRITE:
				hd_read_write(&msg);
				break;

			case DEV_IOCTL:
				hd_ioctl(&msg);
				break;

			default:
				dump_msg("HD driver::unknown msg", &msg);
				spin("FS: :main_loop (invalid msg.type)");
				break;
		}

		ipc_msg_send_rcv(SEND, pid, &msg);
	}
}
