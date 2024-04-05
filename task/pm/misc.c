#include "shared.h"

void proc_cleanup(proc * proc)
{
	message msg2parent 	= {.type = SYSCALL_RET, .PID = get_proc_pid(proc), .STATUS = proc->exit_status};
	ipc_msg_send_rcv(SEND, proc->p_parent, &msg2parent);
	proc->proc_state 	= FREE_SLOT;
}

/* Get linear address of new process, it is also physical address*/
int alloc_mem(int pid, int memsize)
{
	assert(pid >= (NR_TASKS_NATIVE + NR_PROCS_NATIVE));
	if (memsize > PROC_SIZE)
		panic("unsupported memory request: %d. " "(should be less than %d)", memsize, PROC_SIZE);

	int base = PROC_BASE + (pid - (NR_TASKS_NATIVE + NR_PROCS_NATIVE)) * PROC_SIZE;

	if (base + memsize >= memory_size)
		panic("memory allocation failed. pid:%d", pid);

	return base;
}

/* A memory block is dedicated to one and only one PID, no need to "free" any */
int free_mem(int pid)
{
	return 0;
}
