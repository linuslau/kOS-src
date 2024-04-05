#ifndef __KERNEL_SHELL_H
#define __KERNEL_SHELL_H

#define FD_STDIN    0
#define FD_STDOUT   1
#define STDIN_LEN   128
void    shell       (const char * tty_name);
#endif