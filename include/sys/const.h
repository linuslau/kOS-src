#ifndef	_KOS_CONST_H_
#define	_KOS_CONST_H_

/* Color, x is background, y is foreground) */
#define	MAKE_COLOR(x,y)	((x<<4) | y)

#define BLACK   		0x0     /* 		0000 */
#define WHITE   		0x7     /* 		0111 */
#define RED     		0x4     /* 		0100 */
#define GREEN   		0x2     /* 		0010 */
#define BLUE   		 	0x1     /* 		0001 */
#define FLASH   		0x80    /* 1000 0000 */
#define BRIGHT  		0x08    /* 0000 1000 */

/* The number of descriptors in GDT and IDT. */
#define	GDT_SIZE		128
#define	IDT_SIZE		256

/* Privilege */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/* RPL */
#define	RPL_KRNL		SA_RPL0
#define	RPL_TASK		SA_RPL1
#define	RPL_USER		SA_RPL3

/* Process state. */
#define RUNNABLE  		0x00  	/* a process is runable if proc_state is equal to 0 */
#define SENDING   		0x02	/* set when proc trying to send */
#define RECEIVING 		0x04	/* set when proc trying to recv */
#define WAITING   		0x08	/* set when proc waiting for the child to terminate */
#define HANGING   		0x10	/* set when proc exits without being waited by parent */
#define FREE_SLOT 		0x20	/* set when proc table entry is not used */

/* TTY */
#define NR_CONSOLES		3
#define CONSOLE_1 		0
#define CONSOLE_2 		1
#define CONSOLE_3 		2

/* 8259A interrupt controller ports. */
#define	INT_M_CTL		0x20	/* I/O port for interrupt controller         <Master> */
#define	INT_M_CTLMASK	0x21	/* setting bits in this port disables ints   <Master> */
#define	INT_S_CTL		0xA0	/* I/O port for second interrupt controller  <Slave>  */
#define	INT_S_CTLMASK	0xA1	/* setting bits in this port disables ints   <Slave>  */

/* 8253/8254 PIT (Programmable Interval Timer) */
#define TIMER0         	0x40 	/* I/O port for timer channel 0 */
#define TIMER_MODE     	0x43 	/* I/O port for timer mode control */
#define RATE_GENERATOR 	0x34 	/* 00-11-010-0 : * Counter0 - LSB then MSB - rate generator - binary */
#define TIMER_FREQ     	1193182L/* clock frequency for timer in PC and AT */
#define HZ             	100  	/* clock freq (software settable on IBM-PC) */

/* AT keyboard, 8042 ports */
#define KB_DATA			0x60	/* I/O port for keyboard data
								Read : Read Output Buffer
								Write: Write Input Buffer(8042 Data&8048 Command) */
#define KB_CMD			0x64	/* I/O port for keyboard command
								Read : Read Status Register
								Write: Write Input Buffer(8042 Command) */
#define LED_CODE		0xED
#define KB_ACK			0xFA
/* VGA */
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC		/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD		/* reg index of video mem start addr (LSB) */
#define	CURSOR_H		0xE		/* reg index of w_cursor position (MSB) */
#define	CURSOR_L		0xF		/* reg index of w_cursor position (LSB) */
#define	VIDEO_MEM_BASE	0xB8000	/* base of color video memory */
#define	VIDEO_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */
#define SIZE_PER_CONSOLE 		VIDEO_MEM_SIZE / NR_CONSOLES
#define W_SIZE_PER_CONSOLE 		SIZE_PER_CONSOLE >> 1
#define W_TTY_CONSOLE_START(nr_tty)	nr_tty * W_SIZE_PER_CONSOLE

/* CMOS */
#define CLK_ELE			0x70	/* CMOS RAM address register port (write only)
								* Bit 7 = 1  NMI disable
								*	   0  NMI enable
								* Bits 6-0 = RAM address
								*/

#define CLK_IO			0x71	/* CMOS RAM data register port (read/write) */
#define CMOS_READ(reg_addr) 	(out_byte(CLK_ELE, reg_addr), in_byte(CLK_IO))

#define YEAR           	9		/* Clock register addresses in CMOS RAM	*/
#define MONTH          	8
#define DAY            	7
#define HOUR           	4
#define MINUTE         	2
#define SECOND         	0
#define CLK_STATUS    	0x0B	/* Status register B: RTC configuration	*/
#define CLK_HEALTH    	0x0E	/* Diagnostic status: (should be set by Power
									* On Self-Test [POST])
									* Bit  7 = RTC lost power
									*	6 = Checksum (for addr 0x10-0x2d) bad
									*	5 = Config. Info. bad at POST
									*	4 = Mem. size error at POST
									*	3 = I/O board failed initialization
									*	2 = CMOS time invalid
									*    1-0 =    reserved
									*/

/* Hardware interrupts */
#define	NR_IRQ			16
#define	IRQ_TIMER		0
#define	IRQ_KEYBOARD	1
#define	IRQ_CASCADE		2		/* cascade enable for 2nd AT controller */
#define	IRQ_ETHER		3		/* default ethernet interrupt vector */
#define	IRQ_SER_PORT_2	3		/* RS232 interrupt vector for port 2 */
#define	IRQ_SER_PORT_1	4		/* RS232 interrupt vector for port 1 */
#define	IRQ_XT_WINI		5		/* xt winchester */
#define	IRQ_FLOPPY		6
#define	IRQ_PRINTER		7
#define	IRQ_AT_WINI		14		/* at winchester */

/* Tasks, TASK_XX must have the same order in task_table ofglobal.c */
#define INVALID_DRIVER	-20
#define INTERRUPT		-10
#define TASK_TTY		0
#define TASK_SYS		1
#define TASK_HD			2
#define TASK_FS			3
#define TASK_PM			4
#define INIT			5
#define ANY				(NR_TASKS_NATIVE + NR_PROCS_MAX + 10)
#define NO_TASK			(NR_TASKS_NATIVE + NR_PROCS_MAX + 20)

#define	MAX_TICKS		0x7FFFABCD

/* System call */
#define NR_SYS_CALL		1

/* IPC */
#define SEND			1
#define RECEIVE			2
#define SEND_RECEIVE	3

#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'

enum msg_type {
	/* Hard interrupt occurs */
	HW_INT = 1,
	/* SYS task */
	SYS_GET_TICKS, SYS_GET_PID, SYS_GET_RTC_TIME,
	/* FS */
	FS_OPEN, FS_CLOSE, FS_READ, FS_WRITE, FS_LSEEK, FS_STAT, FS_UNLINK,
	/* FS & TTY */
	SUSPEND_PROC, RESUME_PROC, PRINTX,
	/* PM */
	EXEC, WAIT,
	/* FS & PM */
	FORK, EXIT,
	/* TTY, SYS, FS, PM, etc */
	SYSCALL_RET,
	/* Message type for drivers */
	DEV_OPEN = 1001, DEV_CLOSE, DEV_READ, DEV_WRITE, DEV_IOCTL
};

#define	IOCTL_GET_PART					1

/* Hard Drive */
#define SECTOR_SIZE						512 /* size in bytes */
#define SECTOR_BITS						(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT				9 	/* >> SECTOR_SIZE_SHIFT == divided by 512, 2^9 = 512 */

/* Major device numbers (corresponding to kernel/global.c::dev_drv_map[]) */
#define	NR_MAJOR_NO_DEV					0
#define	NR_MAJOR_DEV_FLOPPY				1
#define	NR_MAJOR_DEV_CDROM				2
#define	NR_MAJOR_DEV_HD					3
#define	NR_MAJOR_DEV_CHAR_TTY			4
#define	NR_MAJORDEV_SCSI				5

/* Device number is composed of major and minor numbers */
#define	MAJOR_SHIFT						8
#define	MAKE_DEV_NR(a,b)				((a << MAJOR_SHIFT) | b)

/* Get major and minor numbers from device number */
#define	GET_DEV_MAJOR_NR(x)	((x >> MAJOR_SHIFT) & 0xFF)
#define	GET_DEV_MINOR_NR(x)	(x & 0xFF)

#define	INVALID_INODE					0
#define	ROOT_INODE_NR					1

#define	MAX_DRIVES						2
#define	NR_MAX_PRIM_PART_PER_DRIVE		4
#define	NR_MAX_LOGI_PART_PER_EXT		16
#define	NR_MAX_MINOR_PRIM_PER_DRIVE		(NR_MAX_PRIM_PART_PER_DRIVE + 1)
#define	NR_MAX_MINOR_LOGI_PER_DRIVE		(NR_MAX_PRIM_PART_PER_DRIVE * NR_MAX_LOGI_PART_PER_EXT)

/* If there are 2 disks, prim_dev ranges in hd[0-9], this macro will equals 9. */
#define	NR_MAX_MINOR_PRIM				(NR_MAX_MINOR_PRIM_PER_DRIVE * MAX_DRIVES - 1)
#define	NR_MAX_MINOR_LOGI				(NR_MAX_MINOR_LOGI_PER_DRIVE * MAX_DRIVES)

/* minor device numbers of hard disk, 0~9 is for primary device */
#define	MINOR_NR_hd1a					0x10
#define	MINOR_NR_hd2a					(MINOR_NR_hd1a + NR_MAX_LOGI_PART_PER_EXT)

/* Device number of ROOT DEV */
#define	ROOT_DEV_NR						MAKE_DEV_NR(NR_MAJOR_DEV_HD, NR_MINOR_BOOT_PART)

enum partition_type {
	PRIMARY_PARTITION 	= 0,
	EXTENDED_PARTITION 	= 1
};

#define KOS_PART						0x99
#define NO_PART							0x00
#define EXT_PART						0x05

#define	NR_FILES						64
#define	NR_FILE_DESC					64
#define	NR_INODE						64
#define	NR_SUPER_BLOCK					8

/* INODE::inode_file_type (octal, lower 12 bits reserved) */
#define TYPE_MASK						0170000
#define TYPE_REGULAR    				0100000
#define TYPE_DIRECTORY    				0040000
#define TYPE_SPECIAL_BLOCK 				0060000
#define TYPE_SPECIAL_CHAR 				0020000

#define	NR_FILE_SECTS					2048 /* 2048 * 512 = 1MB */

#define	is_special(file_type)			((((file_type) & TYPE_MASK) == TYPE_SPECIAL_BLOCK) ||	\
						 				(((file_type) & TYPE_MASK) == TYPE_SPECIAL_CHAR))

#endif /* _KOS_CONST_H_ */
