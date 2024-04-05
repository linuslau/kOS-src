#include "shared.h"
#include "hd.h"

void task_fs()
{
	printl("FS:   Task FS is running\n");

	init_fs();

	while (1) {
		ipc_msg_send_rcv(RECEIVE, ANY, &fs_msg);

		int msg_type = fs_msg.type;
		int src = fs_msg.source;
		caller_proc = &proc_table[src];

		switch (msg_type) {
			case FS_OPEN:
				fs_msg.FD = do_open();
				break;
			case FS_CLOSE:
				fs_msg.RETVAL = do_close();
				break;
			case FS_READ:
			case FS_WRITE:
				fs_msg.CNT = do_read_write();
				break;
			case FS_UNLINK:
				fs_msg.RETVAL = do_unlink();
				break;
			case RESUME_PROC:
				src = fs_msg.PROC_NR;
				break;
			case FORK:
				fs_msg.RETVAL = fs_fork();
				break;
			case EXIT:
				fs_msg.RETVAL = fs_exit();
				break;
			case FS_LSEEK:
				fs_msg.OFFSET = do_lseek();
				break;
			case FS_STAT:
				fs_msg.RETVAL = do_stat();
				break;
			default:
				dump_msg("FS: :unknown message:", &fs_msg);
				assert(0);
				break;
		}

#ifdef ENABLE_DISK_LOG
		char * msg_name[128];
		msg_name[FS_OPEN]   = "FS_OPEN";
		msg_name[FS_CLOSE]  = "FS_CLOSE";
		msg_name[FS_READ]   = "FS_READ";
		msg_name[FS_WRITE]  = "FS_WRITE";
		msg_name[FS_LSEEK]  = "FS_LSEEK";
		msg_name[FS_UNLINK] = "FS_UNLINK";
		msg_name[FORK]   = "FORK";
		msg_name[EXIT]   = "EXIT";
		msg_name[FS_STAT]   = "FS_STAT";

		switch (msg_type) {
		case FS_UNLINK:
			dump_fd_graph("%s just finished. (pid:%d)",
				      msg_name[msg_type], src);
			//panic("");
		case FS_OPEN:
		case FS_CLOSE:
		case FS_READ:
		case FS_WRITE:
		case FORK:
		case EXIT:
		case FS_LSEEK:
		case FS_STAT:
			break;
		case RESUME_PROC:
			break;
		default:
			assert(0);
		}
#endif

		/* reply */
		if (fs_msg.type != SUSPEND_PROC) {
			fs_msg.type = SYSCALL_RET;
			ipc_msg_send_rcv(SEND, src, &fs_msg);
		}
	}
}

