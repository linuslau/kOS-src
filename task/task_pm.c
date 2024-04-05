#include "shared.h"

static void init_pm();

/* Ring 1 */
void task_pm()
{
	init_pm();

	while (1) {
		ipc_msg_send_rcv(RECEIVE, ANY, &pm_msg);
		int src = pm_msg.source;
		int reply = 1;

		int msg_type = pm_msg.type;

		switch (msg_type) {
			case FORK:
				pm_msg.RETVAL = do_fork();
				break;
			case EXIT:
				do_exit(pm_msg.STATUS);
				reply = 0;
				break;
			case EXEC:
				pm_msg.RETVAL = do_exec();
				break;
			case WAIT:
				do_wait();
				reply = 0;
				break;
			default:
				dump_msg("PM: :unknown msg", &pm_msg);
				assert(0);
				break;
		}

		if (reply) {
			pm_msg.type = SYSCALL_RET;
			ipc_msg_send_rcv(SEND, src, &pm_msg);
		}
	}
}

static void init_pm()
{
	struct boot_params bp;
	get_boot_params(&bp);
	memory_size = bp.total_ram_size;
	printl("PM:   RAM size: %dMB\n", memory_size / (1024 * 1024));
}

