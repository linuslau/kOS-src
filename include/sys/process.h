
#ifndef	_KOS_PROCESS_H_
#define	_KOS_PROCESS_H_

#define NR_PROCS_MAX		32
#define NR_PROCS_NATIVE		4
#define NR_TASKS_NATIVE		5
#define PROC_FIRST			proc_table[0]
#define PROC_LAST			proc_table[NR_TASKS_NATIVE + NR_PROCS_MAX - 1]

/* Make sure PROC_BASE is higher than any buffers, such as fsbuf, pm_buf.*/
#define	PROC_BASE				0xA00000 /*	10 MB */
#define	PROC_SIZE				0x100000 /* 1 MB */
/* Prepared stack for a new app will be placed here, start with argv, pointer array */
#define	PROC_STACK_RESERVERD	0x400    /*  1 KB */

/* stacks of tasks */
#define	STACK_SIZE_DEFAULT		0x4000 /* 16 KB */
#define STACK_SIZE_TTY			STACK_SIZE_DEFAULT
#define STACK_SIZE_SYS			STACK_SIZE_DEFAULT
#define STACK_SIZE_HD			STACK_SIZE_DEFAULT
#define STACK_SIZE_FS			STACK_SIZE_DEFAULT
#define STACK_SIZE_PM			STACK_SIZE_DEFAULT
#define STACK_SIZE_INIT			STACK_SIZE_DEFAULT
#define STACK_SIZE_APP1			STACK_SIZE_DEFAULT
#define STACK_SIZE_APP2			STACK_SIZE_DEFAULT
#define STACK_SIZE_APP3			STACK_SIZE_DEFAULT

#define STACK_SIZE_TOTAL	   (STACK_SIZE_TTY 	+ 	\
								STACK_SIZE_SYS 	+ 	\
								STACK_SIZE_HD 	+ 	\
								STACK_SIZE_FS 	+ 	\
								STACK_SIZE_PM 	+ 	\
								STACK_SIZE_INIT + 	\
								STACK_SIZE_APP1 + 	\
								STACK_SIZE_APP2 + 	\
								STACK_SIZE_APP3 	)

struct stackframe {	/* proc_ptr points here							↑ Low			*/
	u32	gs;			/* ┓											│				*/
	u32	fs;			/* ┃											│				*/
	u32	es;			/* ┃											│				*/
	u32	ds;			/* ┃											│				*/
	u32	edi;		/* ┃											│				*/
	u32	esi;		/* ┣	pushed by save()						│				*/
	u32	ebp;		/* ┃											│				*/
	u32	kernel_esp;	/*     	'popad' will ignore it					│				*/
	u32	ebx;		/* ┃↑ stack grows downward from high addresses to low addresses.*/
	u32	edx;		/* ┃											│				*/
	u32	ecx;		/* ┃											│				*/
	u32	eax;		/* ┛											│				*/
	u32	retaddr;	/*		return address for save()				│				*/
	u32	eip;		/* ┓											│				*/
	u32	cs;			/* ┃											│				*/
	u32	eflags;		/* ┣ 	these are pushed by CPU during interrupt│				*/
	u32	esp;		/* ┃											│				*/
	u32	ss;			/* ┛											┷ High			*/
};

typedef struct process {
	struct 			stackframe regs;    			/* process registers saved in stack frame */
	u16 			ldt_sel;               			/* gdt selector giving ldt base and limit */
	struct 			gdt_desc ldts[LDT_SIZE]; 		/* local descs for code and data */
	file_desc * 	file_desc[NR_FILES];
	int 			ticks;                 			/* remained ticks */
	int 			priority;
	char 			name[16];		  		 		/* name of the process */
	int  			proc_state;						/* Check Process state in const.h */

	message * 		pending_msg;
	struct process* q_pending_proc;
	int				pid_recvingfrom;
	int	 			pid_sendingto;
	int 			has_int_msg;

	int 			p_parent;
	int 			exit_status; 					/* For parent */
} proc;

typedef	void	(*task_f)	();
struct task {
	task_f	initial_eip;
	int		stacksize;
	char	name[32];
};

#define get_proc_pid(proc) (proc - proc_table)
struct boot_params {
	int				total_ram_size;
	unsigned char *	kernel_start_addr;
};

/* GNU tar*/
struct posix_tar_header
{
	char name		[100];		/*   0 */
	char mode		[8];		/* 100 */
	char uid		[8];		/* 108 */
	char gid		[8];		/* 116 */
	char size		[12];		/* 124 */
	char mtime		[12];		/* 136 */
	char chksum		[8];		/* 148 */
	char typeflag;				/* 156 */
	char linkname	[100];		/* 157 */
	char magic		[6];		/* 257 */
	char version	[2];		/* 263 */
	char uname		[32];		/* 265 */
	char gname		[32];		/* 297 */
	char devmajor	[8];		/* 329 */
	char devminor	[8];		/* 337 */
	char prefix		[155];		/* 345 */
	/* 500 */
};

#endif /* _KOS_PROCESS_H_ */