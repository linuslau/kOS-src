/* kliba.asm */
void	out_byte			(u16 port, u8 value);
u8		in_byte				(u16 port);
void	disp_str			(char * info);
void	disp_color_str		(char * info, int color);
void	disable_irq			(int irq);
void	enable_irq			(int irq);
void	disable_int			();
void	enable_int			();
void	port_read			(u16 port, void* buf, int n);
void	port_write			(u16 port, void* buf, int n);
void	glitter				(int row, int col);

/* string.asm */
char*	strcpy				(char* dst, const char* src);

/* gdt_desc.c */
void	init_descriptors	();
void	init_idt_desc		();
void	init_tss_desc		();
void	init_ldt_desc		();
int		ldt_idx_to_base_addr(proc* p, int ldt_index);
u32		gdt_seg_to_base_addr(u16 seg);
u32 	gdt_ldt_to_base_addr(struct gdt_desc* p_desc);
void	make_gdt_desc		(struct gdt_desc * p_desc, u32 base, u32 limit, u16 attribute);
void	make_idt_desc		(unsigned char vector, u8 desc_type, int_handler handler, unsigned char privilege);

/* klib.c */
void	get_boot_params		(struct boot_params * pbp);
int		get_kernel_mem_range(unsigned int * b, unsigned int * l);
void	delay				(int time);
void	disp_int			(int input);
char *	itoa				(char * str, int num);

/* kernel.asm */
void 	switch_to_user_mode	();
void 	load_gdtr_ldtr_tr	();

/* main.c */
void 	Init				();
int  	get_ticks			();
void 	APP1				();
void 	APP2				();
void 	APP3				();
void 	panic				(const char *fmt, ...);

/* 8259.c */
void 	init_8259A			();
void 	set_irq_handler		(int irq, irq_handler handler);
void 	default_irq_handler	(int irq);
void 	set_default_irq_handler();

/* clock.c */
void 	clock_handler		(int irq);
void 	init_clock			();
void 	m_delay				(int milli_sec);

/* kernel/hd.c */
void 	task_hd				();
void 	hd_handler			(int irq);

/* keyboard.c */
void 	init_keyboard		();
void 	keyboard_read		(TTY* p_tty);

/* task_tty */
void 	task_tty			();
void	tty_process_key		(TTY* p_tty, u32 key);
void 	wake_up_tty			();
int  	sys_printx			(int _unused1, int _unused2, char* s, proc* p_proc);

/* task_sys.c */
void 	task_sys			();

/* fs/main.c */
void 	task_fs				();
int		rd_wr_sector		(int io_type, int dev_nr, u64 pos, int bytes, int proc_nr, void * buf);
inode*	get_file_inode		(int dev_nr, int num);
void	put_inode			(inode * pinode);
void	write_inode			(inode * p);
super_block*get_super_block	(int dev_nr);

/* fs/open.c */
int		do_open				();
int		do_close			();
int		do_lseek			();

/* fs/read_write.c */
int		do_read_write		();

/* fs/unlink.c */
int		do_unlink			();

/* fs/misc.c */
int		do_stat				();
int 	get_file_name		(const char * full_path, char * file_name, inode** p_inode);
int		get_file_inode_nr	(char * path);

/* fs/disklog.c */
int		disklog				(char * logstr); /* for debug */
void	dump_fd_graph		(const char * fmt, ...);

/* mm/main.c */
void	task_pm				();

int		alloc_mem			(int pid, int memsize);
int		free_mem			(int pid);
void 	proc_cleanup		(proc * proc);

/* mm/fork.c */
int		do_fork				();
void	do_exit				(int status);
void	do_wait				();

/* mm/exec.c */
int		do_exec				();

/* console.c */
void 	console_print_char	(CONSOLE* p_con, char ch);
void 	scroll_screen		(CONSOLE* p_con, int direction);
void 	switch_console		(int nr_console);
void 	init_screen			(TTY* p_tty);
int  	is_current_console	(CONSOLE* p_con);

/* process.c */
void 	init_process		();
void	schedule			();
void*	proc_va2la			(int pid, void* va);
void	reset_msg			(message* p);
void	dump_msg			(const char * title, message* m);
void	dump_proc			(proc * p);
int		ipc_msg_send_rcv	(int function, int src_dest, message* msg);
void	hw_int_notification	(int task_nr);

/* lib/misc.c */
void 	spin				(char * func_name);

/* syscall.asm */
void    sys_call			();

/* system call ring3 in syscall.asm */
int		sys_send_rcv		(int function, int src_dest, message* pending_msg);

/* system call ring0 in ipc.c */
int		sys_send_rcv_handler(int function, int src_dest, message* m, proc* p);

void 	shell				(const char * tty_name);
