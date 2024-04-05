#include "shared.h"

/* Check README.md for detail */
void do_exit(int status)
{
	int current_pid = pm_msg.source; /* PID of caller */
	int parent_pid = proc_table[current_pid].p_parent;
	proc * p = &proc_table[current_pid];

	message msg2fs = {.type = EXIT, .PID = current_pid};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg2fs);

	free_mem(current_pid);

	p->exit_status = status;

	if (proc_table[parent_pid].proc_state & WAITING) {
		/* parent is waiting, cleanup the process */
		proc_table[parent_pid].proc_state &= ~WAITING;
		proc_cleanup(&proc_table[current_pid]);
	} else { /* parent is not waiting */
		/* HANGING: everything except the proc_table entry has been cleaned up. */
		/* cleanup will happen in wait.c when HANGING is checked*/
		proc_table[current_pid].proc_state |= HANGING;
	}

	/* if the proc has a child, make hen (INIT process) the new parent */
	for (int i = 0; i < NR_TASKS_NATIVE + NR_PROCS_MAX; i++) {
		if (proc_table[i].p_parent == current_pid) { /* is a child */
			proc_table[i].p_parent = INIT;
			if ((proc_table[INIT].proc_state & WAITING) &&
			    (proc_table[i].proc_state & HANGING)) {
				proc_table[INIT].proc_state &= ~WAITING;
				proc_cleanup(&proc_table[i]);
			}
		}
	}
}
