#define GLOBAL_VARIABLE

#include "shared.h"

char			task_stack[STACK_SIZE_TOTAL];
TTY				tty_table[NR_CONSOLES];
CONSOLE			console_table[NR_CONSOLES];
proc 			proc_table[NR_TASKS_NATIVE + NR_PROCS_MAX];
irq_handler		irq_table[NR_IRQ];
system_call		sys_call_table[NR_SYS_CALL] = {sys_send_rcv_handler};

/* Ring 1, task_xxx must have the same order in #define TASK_XX of const.h */
struct task	task_table[NR_TASKS_NATIVE] = {
	/* entry        stack size        	task name */
	{task_tty,      STACK_SIZE_TTY,   	"TTY"	},
	{task_sys,      STACK_SIZE_SYS,   	"SYS"   },
	{task_hd,       STACK_SIZE_HD,    	"HD"    },
	{task_fs,       STACK_SIZE_FS,    	"FS"    },
	{task_pm,       STACK_SIZE_PM,    	"PM"    }};

/* Ring 3 */
struct task	user_proc_table[NR_PROCS_NATIVE] = {
	/* entry    	stack size     		proc name */
	{Init,			STACK_SIZE_INIT,	"INIT"},
	{APP1,  		STACK_SIZE_APP1, 	"APP1"},
	{APP2,  		STACK_SIZE_APP2, 	"APP2"},
	{APP3,  		STACK_SIZE_APP3, 	"APP3"}};
	
/* In accordance with include/const.h accordingly */
device_driver_map dev_drv_map[] = {
	/* driver nr.			major device nr. */
	{INVALID_DRIVER},		/* 0 : Unused */
	{INVALID_DRIVER},		/* 1 : Reserved for floppy driver */
	{INVALID_DRIVER},		/* 2 : Reserved for cdrom driver */
	{TASK_HD},				/* 3 : Hard disk */
	{TASK_TTY},				/* 4 : TTY */
	{INVALID_DRIVER}		/* 5 : Reserved for scsi disk driver */
};

/* 6MB~7MB: buffer for FS */
u8 *		fsbuf			= (u8*)0x600000;
const int	FS_BUF_SIZE		= 0x100000;

/* 7MB~8MB: buffer for PM */
u8 *		pm_buf			= (u8*)0x700000;
const int	PM_BUF_SIZE		= 0x100000;

/* 8MB~10MB: buffer for log (debug) */
char *		logbuf			= (char*)0x800000;
const int	LOG_BUF_SIZE	= 0x100000;
char *		logdiskbuf		= (char*)0x900000;
const int	LOGDISKBUF_SIZE	= 0x100000;