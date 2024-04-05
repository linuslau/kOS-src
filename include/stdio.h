#ifndef	_KOS_STDIO_H_
#define	_KOS_STDIO_H_

#include "stdint.h"

#define NULL 	((void*)0)
#define bool 	int
#define true 	1
#define false	0

typedef	char *	va_list;

#define ASSERT
#ifdef ASSERT
void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)  if (exp) ; else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

/* EXTERN */
#define	EXTERN				extern	/* EXTERN is defined as extern except in global.c */

/* string */
#define	STR_BUFFER_LEN		1024

#define	O_CREAT				1
#define	O_RDWR				2
#define	O_TRUNC				4

#define SEEK_SET			1
#define SEEK_CUR			2
#define SEEK_END			3

#define	MAX_PATH			128
#define	MAX_FILENAME_LEN 	12

#define	BCD_TO_DEC(x)      ( (x >> 4) * 10 + (x & 0x0f) )

struct stat {
	int st_dev_nr;		/* major/minor device number */
	int st_inode_nr;	/* i-node number */
	int st_file_mode;	/* file mode, protection bits, etc. */
	int st_dev_nr_s;	/* device nr (if special file), check dev_tty0~2 inode_sect_start assignment in mkfs() of fs.c */
	int st_file_size;	/* file size */
};

struct rtc_time {
	u32 year;
	u32 month;
	u32 day;
	u32 hour;
	u32 minute;
	u32 second;
};

/* printf.c */
int	printf(const char *fmt, ...);
int printl(const char *fmt, ...);

/* vsprintf.c */
int	vsprintf(char *buf, const char *fmt, va_list args);
int	sprintf(char *buf, const char *fmt, ...);

/* printf vs. printl vs. printx
 *   printf:
 *           [send msg]             FS_WRITE         DEV_WRITE
 *           USER_PROC ------------→FS -------------→TTY
                     ↖______________↙↖_______________/
 *           [recv msg]             SYSCALL_RET      SYSCALL_RET
 *
 *   printl: variant-parameter-version printx
 *           calls vsprintf, inturn call printx (trap into kernel)
 *
 *   printx: low level print without using IPC (eventually, replaced by IPC to make it unified)
 *           USER_PROC -- -- -- -- -- --> KERNEL
 *                       trap directly
 */

/* max() & min() */
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	min(a,b)	((a) < (b) ? (a) : (b))

/* Lib Functions */
#ifdef ENABLE_DISK_LOG
#define SYSLOG syslog
#endif

/* lib/open.c */
int	open		(const char *pathname, int flags);

/* lib/close.c */
int	close		(int fd);

/* lib/read.c */
int	read		(int fd, void *buf, int count);

/* lib/write.c */
int	write		(int fd, const void *buf, int count);

/* lib/lseek.c */
int	lseek		(int fd, int offset, int whence);

/* lib/unlink.c */
int	unlink		(const char *pathname);

/* lib/getpid.c */
int	getpid		();

/* lib/fork.c */
int	fork		();

/* lib/exit.c */
void	exit	(int status);

/* lib/wait.c */
int	wait		(int * status);

/* lib/exec.c */
int	exec		(const char * path);
int	execl		(const char * path, const char *arg, ...);
int	execv		(const char * path, char * argv[]);

/* lib/stat.c */
int	stat		(const char *path, struct stat *buf);

/* lib/syslog.c */
int	syslog		(const char *fmt, ...);

#endif /* _KOS_STDIO_H_ */
