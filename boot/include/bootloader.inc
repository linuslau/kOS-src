
STACK_BOTTOM_BOOT		equ	0x7C00	; base address of stack when booting
STACK_BOTTOM_LOADER		equ	0x0100	; base address of stack in loader
TRANSFER_SECT_NR	    equ	2       ; Nr of blocks to transfer, check "disk_addr_packet"
SECT_BUF_SIZE		    equ	TRANSFER_SECT_NR * 512

; kernel entry point, corresponding with Makefile
KRNL_ENT_PT_PHY_ADDR	equ	0x1000

; paging
PAGE_DIR_BASE		    equ	0x100000
PAGE_TABLE_BASE	    	equ	0x101000
PAGE_TABLE_SIZE         equ 4096*1024
PAGE_SIZE               equ 4096
PAGE_TABLE_ENTRIES      equ 1024
PAGE_CR0_PG_BIT         equ 80000000h

; where loader is loaded
LOADER_SEG		        equ	0x9000
LOADER_OFFSET		    equ	0x100
LOADER_ADDR		        equ	LOADER_SEG << 4

; where kernel file is loaded
KERNEL_SEG		        equ	0x7000
KERNEL_OFFSET		    equ	0
KERNEL_ADDR	            equ	KERNEL_SEG << 4

; bytes reserved for kernel.bin
VALID_KERNEL_SIZE	    equ	LOADER_ADDR - KERNEL_ADDR

; super block will be stored at: [0x700,0x900)
SUPER_BLK_SEG		    equ	0x70

BOOT_PARAM_ADDR		    equ	0x900
BOOT_PARAM_MAGIC	    equ	0xB007

; starting/first sector of this partition where OS is installed
ROOT_BASE	    	    equ	0x4EFF

; pair with "super_block" in include/sys/fs.h or check struc below
SB_MAGIC_V1		        equ	0x111
SB_MAGIC	        	equ	4 *  0
SB_NR_INODES	    	equ	4 *  1
SB_NR_SECTS		        equ	4 *  2
SB_NR_IMAP_SECTS	    equ	4 *  3
SB_NR_SMAP_SECTS    	equ	4 *  4
SB_N_1ST_SECT		    equ	4 *  5
SB_NR_INODE_SECTS	    equ	4 *  6
SB_ROOT_INODE_NR		equ	4 *  7
SB_INODE_SIZE		    equ	4 *  8
SB_INODE_ISIZE_OFF	    equ	4 *  9
SB_INODE_START_OFF      equ	4 *  10
SB_DIR_ENT_SIZE		    equ	4 *  11
SB_DIR_ENT_INODE_OFF	equ	4 *  12
SB_DIR_ENT_FNAME_OFF	equ	4 *  13

; reference
; typedef struct {
; 	u32	magic;		 		/* Magic number */
; 	u32	nr_inodes;	 	 	/* How many inodes */
; 	u32	nr_sects;	  		/* How many sectors */
; 	u32	nr_inode_map_sects;	/* How many inode-map sectors */
; 	u32	nr_sect_map_sects;	/* How many sector-map sectors */
; 	u32	nr_1st_data_sect;	/* Sector number of the 1st data sector */
; 	u32	nr_inode_sects;   	/* How many inode sectors */
; 	u32	root_inode;       	/* Inode nr of root directory */
; 	u32	inode_size;       	/* INODE_SIZE */
; 	u32	inode_isize_off;  	/* Offset of 'inode::inode_file_size' */
; 	u32	inode_start_off;  	/* Offset of 'inode::inode_sect_start' */
; 	u32	dir_ent_size;     	/* DIR_ENTRY_SIZE */
; 	u32	dir_ent_inode_off;	/* Offset of 'dir_entry::inode_nr' */
; 	u32	dir_ent_fname_off;	/* Offset of 'dir_entry::name' */
;
;   int	sb_dev; 			/* the super block's home device */
; 							/* sb_dev is only present in memory */
; } super_block;

; typedef {
; 	u32	inode_file_type;	/* Accsess mode */
; 	u32	inode_file_size;	/* File size */
; 	u32	inode_sect_start;	/* The first sector of the data */
; 	u32	inode_nr_sects;		/* How many sectors the file occupies */
	; u8	_unused[16];		/* Stuff for alignment */

; 	/* the following items are only present in memory */
; 	int	inode_dev;			/* device nr*/
; 	int	inode_cnt;			/* How many procs share this inode */
; 	int	inode_nr;			/* inode nr. */
; } inode;
;
; typedef struct directory_entry {
; 	int		inode_nr;
; 	char	name[MAX_FILENAME_LEN];
; } dir_entry;