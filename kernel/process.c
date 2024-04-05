#include "shared.h"

static void run_1st_process	();
static void untar			(const char * filename);

void init_process()
{
	int i, j, eflags, prio;
	u8  rpl;
	u8  priv;

	struct task * task;
	proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS_NATIVE + NR_PROCS_MAX; i++,p++,task++) {
		if (i >= NR_TASKS_NATIVE + NR_PROCS_NATIVE) {
			p->proc_state = FREE_SLOT;
			continue;
		}

		if (i < NR_TASKS_NATIVE) {     /* TASK */
			task	= task_table + i;
			priv	= PRIVILEGE_TASK;
			rpl     = RPL_TASK;
			eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
		} else {                  /* USER PROC */
			task	= user_proc_table + (i - NR_TASKS_NATIVE);
			priv	= PRIVILEGE_USER;
			rpl     = RPL_USER;
			eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
		}

		strcpy(p->name, task->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(task->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_mem_range(&k_base, &k_limit);
			assert(ret == 0);
			make_gdt_desc(&p->ldts[INDEX_LDT_C],
							0, /* bytes before the entry point
								* are useless (wasted) for the
								* INIT process, doesn'task matter
								*/
							(k_base + k_limit) >> LIMIT_4K_SHIFT,
							DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			make_gdt_desc(&p->ldts[INDEX_LDT_RW],
							0, /* bytes before the entry point
								* are useless (wasted) for the
								* INIT process, doesn'task matter
								*/
							(k_base + k_limit) >> LIMIT_4K_SHIFT,
							DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs 			= INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds 			=
		p->regs.es 			=
		p->regs.fs			=
		p->regs.ss 			= INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs 			= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip			= (u32)task->initial_eip;
		p->regs.esp			= (u32)stk;
		p->regs.eflags		= eflags;

		p->ticks 			= p->priority = prio;

		p->proc_state 		= RUNNABLE;
		p->pending_msg 		= 0;
		p->pid_recvingfrom 	= NO_TASK;
		p->pid_sendingto 	= NO_TASK;
		p->has_int_msg 		= 0;
		p->q_pending_proc 	= 0;

		for (j = 0; j < NR_FILES; j++)
			p->file_desc[j] = 0;

		stk -= task->stacksize;
	}

	nested_int 		= 0;
	sys_ticks 		= 0;

	p_proc_ready	= proc_table;
	run_1st_process();
}

void run_1st_process()
{
	switch_to_user_mode();
}

/* The hen. */
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	assert(fd_stdout == 1);

	printf("\nInit: First process is running\n");
	
	untar("/cmd.tar");
	char * tty_list[] = {"/dev_tty1", "/dev_tty2"};

	printf("\n");
	for (int i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("Init: Parent is running, forked child pid:%d\n", pid);
		} else {	/* child process */
			printf("Init: Child  is running, pid:%d\n", getpid());
			close(fd_stdin);
			close(fd_stdout);
			shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}

void APP1()
{
	for(;;);
}

void APP2()
{
	for(;;);
}

void APP3()
{
	for(;;);
}

/* Ring 0, choose one proc to run. */
void schedule()
{
	proc* 	p;
	int		greatest_ticks = 0;

	while (!greatest_ticks) {
		for (p = &PROC_FIRST; p <= &PROC_LAST; p++) {
			if (p->proc_state == RUNNABLE) {
				if (p->ticks > greatest_ticks) {
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
			}
		}

		if (!greatest_ticks)
			for (p = &PROC_FIRST; p <= &PROC_LAST; p++)
				if (p->proc_state == RUNNABLE)
					p->ticks = p->priority;
	}
}

void* proc_va2la(int pid, void* va)
{
	proc* p = &proc_table[pid];

	u32 seg_base_addr = ldt_idx_to_base_addr(p, INDEX_LDT_RW);
	u32 la = seg_base_addr + (u32)va;

	if (pid < NR_TASKS_NATIVE + NR_PROCS_NATIVE) {
		assert(la == (u32)va);
	}

	return (void*)la;
}

void untar(const char * filename)
{
	printf("Init: Extracting `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	int i = 0;
	int bytes = 0;

	while (1) {
		bytes = read(fd, buf, SECTOR_SIZE);
		assert(bytes == SECTOR_SIZE); /* size of a TAR file must be times of 512 */
		if (buf[0] == 0) {
			if (i == 0)
				printf("      %s has already extracted, no need to unpack again.\n", filename);
			break;
		}
		i++;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]\n");
			close(fd);
			return;
		}
		printf("      %d: %s\n", i, phdr->name);
		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			bytes = write(fdout, buf, iobytes);
			assert(bytes == iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	if (i) {
		lseek(fd, 0, SEEK_SET);
		buf[0] = 0;
		bytes = write(fd, buf, 1);
		assert(bytes == 1);
		printf("Init: Extracting done, %d files extracted\n", i);
	}

	close(fd);
}