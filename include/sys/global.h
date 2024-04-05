/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int	                sys_ticks;
EXTERN	int	                disp_pos;

EXTERN	u8					gdtr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	struct gdt_desc		gdt[GDT_SIZE];
EXTERN	u8					idtr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	struct gate_desc	idt[IDT_SIZE];

EXTERN	u32	                nested_int;
EXTERN	int	                current_console;

EXTERN	int	                key_pressed;

EXTERN	struct tss		    tss;
EXTERN	proc *	            p_proc_ready;

extern	char			    task_stack[];
extern	proc		        proc_table[];
extern  struct task		    task_table[];
extern  struct task		    user_proc_table[];
extern	irq_handler		    irq_table[];
extern	TTY				    tty_table[];
extern  CONSOLE			    console_table[];

/* PM */
EXTERN	message			    pm_msg;
extern	u8 *			    pm_buf;
extern	const int		    PM_BUF_SIZE;
EXTERN	int				    memory_size;

/* FS */
EXTERN	file_desc		    file_desc_table[NR_FILE_DESC];
EXTERN	inode			    inode_table[NR_INODE];
EXTERN	super_block         super_block_table[NR_SUPER_BLOCK];
extern	u8 *			    fsbuf;
extern	const int		    FS_BUF_SIZE;
EXTERN	message			    fs_msg;
EXTERN	proc *	            caller_proc;
EXTERN	inode *			    root_inode;
extern	device_driver_map	dev_drv_map[];

/* Disk log */
extern	char *			    logbuf;
extern	const int		    LOG_BUF_SIZE;
extern	char *			    logdiskbuf;
extern	const int		    LOGDISKBUF_SIZE;
