#include "shared.h"
#include "shell.h"

void shell(const char * tty_name)
{
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[STDIN_LEN];

	while (1) {
		write(FD_STDOUT, "[kz@localhost /]$ ", 17);
		int r = read(FD_STDIN, rdbuf, STDIN_LEN);
		rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_STACK_RESERVERD];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;

		int fd = open(argv[0], O_RDWR);
		if (fd == -1) {
			/* file open failed, cmd is not available and echo command right away. */
			if (rdbuf[0]) {
				write(FD_STDOUT, "\"", 1);
				write(FD_STDOUT, rdbuf, r);
				write(FD_STDOUT, "\" not found \n", 13);
			}
		} else {
			close(fd);
			/* copy everything from parent to child proc */
			int pid = fork();
			if (pid != 0) { /* parent */
				int s;
				wait(&s);
			} else {	/* child */
				/* argv is command and parameters*/
				/* till now, child is still the same as parent*/
				execv(argv[0], argv);
				/* till now, child is replaced with new command and continue run*/
			}
		}
	}

	close(FD_STDIN);
	close(FD_STDOUT);
}
