#include "shared.h"

int exec(const char * path)
{
	message msg;
	msg.type		= EXEC;
	msg.PATHNAME	= (void*)path;
	msg.NAME_LEN	= strlen(path);
	msg.BUF			= 0;
	msg.BUF_LEN		= 0;

	ipc_msg_send_rcv(SEND_RECEIVE, TASK_PM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}

int execl(const char *path, const char *arg, ...)
{
	va_list parg = (va_list)(&arg);
	char **p = (char**)parg;
	return execv(path, p);
}

/* argv is command and parameters */
int execv(const char *path, char * argv[])
{
	char **p = argv;
	char argv_stack[PROC_STACK_RESERVERD];
	int argv_stack_len = 0;

	/* Iterate through the parameters passed to itself by its callers (such as execl()),
	and count the number of parameters. */
	while(*p++) {
		assert(argv_stack_len + 2 * sizeof(char*) < PROC_STACK_RESERVERD);
		argv_stack_len += sizeof(char*);
	}

	/* Assign zero to the end of the pointer array. */
	*((int*)(&argv_stack[argv_stack_len])) = 0;
	argv_stack_len += sizeof(char*);

	char ** q = (char**)argv_stack;
	/* Iterate through all the strings. */
	for (p = argv; *p != 0; p++) {
		/* Write the address of each string to the correct position in the pointer array. */
		*q++ = &argv_stack[argv_stack_len];

		assert(argv_stack_len + strlen(*p) + 1 < PROC_STACK_RESERVERD);
		/* Copy the string to argv_stack[].*/
		strcpy(&argv_stack[argv_stack_len], *p);
		argv_stack_len += strlen(*p);
		argv_stack[argv_stack_len] = 0;
		argv_stack_len++;
	}

	message msg;
	msg.type	= EXEC;
	msg.PATHNAME	= (void*)path;
	msg.NAME_LEN	= strlen(path);
	msg.BUF		= (void*)argv_stack;
	msg.BUF_LEN	= argv_stack_len;

	ipc_msg_send_rcv(SEND_RECEIVE, TASK_PM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}

