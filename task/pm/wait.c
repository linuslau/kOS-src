#include "shared.h"

/* Check README.md for more detail */
void do_wait()
{
	int pid 			= pm_msg.source;
	int has_children 	= 0;
	proc* p_proc = proc_table;

	for (int i = 0; i < NR_TASKS_NATIVE + NR_PROCS_MAX; i++,p_proc++) {
		if (p_proc->p_parent == pid) {
			has_children++;
			if (p_proc->proc_state & HANGING) {
				proc_cleanup(p_proc);
				return;
			}
		}
	}

	if (has_children) {
		/* has children, but no child is HANGING */
		proc_table[pid].proc_state |= WAITING;
	} else {
		/* no child at all */
		message msg = {SYSCALL_RET, .PID = NO_TASK};
		ipc_msg_send_rcv(SEND, pid, &msg);
	}
}
