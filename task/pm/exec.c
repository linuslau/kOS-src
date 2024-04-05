#include "shared.h"
#include "elf.h"

int do_exec()
{
	int name_len = pm_msg.NAME_LEN;
	int exec_pid = pm_msg.source;
	assert(name_len < MAX_PATH);

	char pathname[MAX_PATH];
	phys_copy((void*)proc_va2la(TASK_PM, pathname),
			  (void*)proc_va2la(exec_pid, pm_msg.PATHNAME),
			  name_len);
	pathname[name_len] = 0;	/* end of the string */

	/* get the file size */
	struct stat s;
	int ret = stat(pathname, &s);
	if (ret != 0) {
		printl("PM:   PM: :do_exec()::stat() returns error. %s", pathname);
		return -1;
	}

	/* read the file */
	int fd = open(pathname, O_RDWR);
	if (fd == -1)
		return -1;
	assert(s.st_file_size < PM_BUF_SIZE);
	read(fd, pm_buf, s.st_file_size);
	close(fd);

	/* Parse new app elf and overwrite the current proc image with the new one */
	Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)(pm_buf);
	for (int i = 0; i < elf_hdr->e_phnum; i++) {
		Elf32_Phdr* prog_hdr = (Elf32_Phdr*)(pm_buf + elf_hdr->e_phoff + (i * elf_hdr->e_phentsize));
		if (prog_hdr->p_type == PT_LOAD) {
			assert(prog_hdr->p_vaddr + prog_hdr->p_memsz < PROC_SIZE);
			phys_copy((void*)proc_va2la(exec_pid, (void*)prog_hdr->p_vaddr),
					  (void*)proc_va2la(TASK_PM, pm_buf + prog_hdr->p_offset),
					  prog_hdr->p_filesz);
		}
	}

	/* setup the arg stack */
	int argv_stack_len = pm_msg.BUF_LEN;
	char stackcopy[PROC_STACK_RESERVERD];
	phys_copy((void*)proc_va2la(TASK_PM, stackcopy),
			  (void*)proc_va2la(exec_pid, pm_msg.BUF),
			  argv_stack_len);

	u8 * stack_pointer = (u8*)(PROC_SIZE - PROC_STACK_RESERVERD);

	/* Note, pm_msg.BUF is equal to msg.BUF in functoin execv of lib/pm/exec.c 
	As pm_msg is copied from msg, pm_msg has a new address in PM process, not pm_msg.BUF
	we are still manipulating child arg_stack*/
	int delta = (int)stack_pointer - (int)pm_msg.BUF;

	int argc = 0;
	/* String like echo/hello/world in stackcopy starts with msg.BUF/pm_msg.BUF
	it will be rebased to stack_pointer, so add this delta, pointer of string address needs to be changed*/
	if (argv_stack_len) {	/* has args */
		char **q = (char**)stackcopy;
		for (; *q != 0; q++,argc++)
			/* *q is the pointer of pointer array, each pointer is a string terminated with 0 */
			*q += delta;
	}

	/* Copy back to child process and put stack to the right position (stack_pointer)*/
	phys_copy((void*)proc_va2la(exec_pid, stack_pointer),
			  (void*)proc_va2la(TASK_PM, stackcopy),
		      argv_stack_len);

	/* Check start.asm file for register eax and ecx*/
	proc_table[exec_pid].regs.ecx = argc; /* argc */
	proc_table[exec_pid].regs.eax = (u32)stack_pointer; /* argv */

	/* setup eip & esp */
	proc_table[exec_pid].regs.eip = elf_hdr->e_entry; /* @see _start.asm */
	proc_table[exec_pid].regs.esp = PROC_SIZE - PROC_STACK_RESERVERD;

	strcpy(proc_table[exec_pid].name, pathname);

	return 0;
}
