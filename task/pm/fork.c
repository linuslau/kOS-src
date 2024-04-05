#include "shared.h"

int do_fork()
{
	int child_pid, parent_pid;
	u16 child_ldt_sel;
	proc* child_proc = proc_table;
	parent_pid = pm_msg.source;

	/* find a free slot in proc_table */
	for (child_pid = 0; child_pid < NR_TASKS_NATIVE + NR_PROCS_MAX; child_pid++,child_proc++)
		if (child_proc->proc_state == FREE_SLOT)
			break;
	if (child_pid == NR_TASKS_NATIVE + NR_PROCS_MAX) /* no free slot */
		return -1;

	child_ldt_sel = child_proc->ldt_sel;
	/* duplicate the process table */
	/* Note, it is *child_proc assignment, not child_proc = proc_table[pid]; child_proc is still child proc, not proc pid*/
	*child_proc = proc_table[parent_pid];
	child_proc->ldt_sel = child_ldt_sel;
	child_proc->p_parent = parent_pid;
	sprintf(child_proc->name, "%s_%d", proc_table[parent_pid].name, child_pid);

	/* duplicate the process: T, D & S */
	struct gdt_desc * p_desc_in_ldt;

	/* Text segment */
	p_desc_in_ldt = &proc_table[parent_pid].ldts[INDEX_LDT_C];
	/* base of T-seg, in bytes */
	int caller_T_base  = gdt_ldt_to_base_addr(p_desc_in_ldt);
	/* limit of T-seg, in 1 or 4096 bytes, depending on the G bit of gdt_desc */
	int caller_T_limit = reassembly(0, 0,
					(p_desc_in_ldt->limit_high_attr2 & 0xF), 16,
					p_desc_in_ldt->limit_low);
	/* size of T-seg, in bytes */
	int caller_T_size  = ((caller_T_limit + 1) *
			      ((p_desc_in_ldt->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
			       4096 : 1));

	/* Data & Stack segments */
	p_desc_in_ldt = &proc_table[parent_pid].ldts[INDEX_LDT_RW];
	/* base of D&S-seg, in bytes */
	int caller_D_S_base  = gdt_ldt_to_base_addr(p_desc_in_ldt);
	/* limit of D&S-seg, in 1 or 4096 bytes,
	   depending on the G bit of gdt_desc */
	int caller_D_S_limit = reassembly((p_desc_in_ldt->limit_high_attr2 & 0xF), 16,
					  0, 0,
					  p_desc_in_ldt->limit_low);
	/* size of D&S-seg, in bytes */
	int caller_D_S_size  = ((caller_T_limit + 1) *
				((p_desc_in_ldt->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
				 4096 : 1));

	/* we don't separate T, D & S segments, so we have: */
	assert((caller_T_base  == caller_D_S_base ) &&
	       (caller_T_limit == caller_D_S_limit) &&
	       (caller_T_size  == caller_D_S_size ));

	/* base of child proc, T, D & S segments share the same space, so we allocate memory just once */
	int new_child_base = alloc_mem(child_pid, caller_T_size);

	/* child is a copy of the parent */
	phys_copy((void*)new_child_base, (void*)caller_T_base, caller_T_size);

	/* Fill child's LDT */
	make_gdt_desc(&child_proc->ldts[INDEX_LDT_C],
		  new_child_base,
		  (PROC_SIZE - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
	make_gdt_desc(&child_proc->ldts[INDEX_LDT_RW],
		  new_child_base,
		  (PROC_SIZE - 1) >> LIMIT_4K_SHIFT,
		  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

	/* tell FS, see fs_fork() in task_fs.c */
	message msg2fs = {.type = FORK, .PID = child_pid};
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_FS, &msg2fs);

	/* parent proc gets a new PID, child proc gets PID 0, so process knows who I am after fork() */
	/* child PID will be returned to the parent proc */
	pm_msg.PID = child_pid;

	/* birth of the child */
	message m = {.type = SYSCALL_RET, .RETVAL = 0, .PID = 0};
	ipc_msg_send_rcv(SEND, child_pid, &m);

	return 0;
}
