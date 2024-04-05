#ifndef	_KOS_FS_H_
#define	_KOS_FS_H_

void 	init_fs				();
void 	mkfs				();
int		fs_fork				();
int 	fs_exit				();
void 	read_super_block	(int dev_nr);

#define	MAX_FILENAME_LEN	12
#define	DIR_ENTRY_SIZE		sizeof(dir_entry)

typedef struct {
	int driver_nr;
} device_driver_map;

#define	MAGIC_V1			0x111

/* from magic to dir_ent_fname_off of struct super_block, not include sb_dev */
#define	SUPER_BLOCK_SIZE	56
typedef struct {
	u32	magic;		 		/* Magic number */
	u32	nr_inodes;	 	 	/* How many inodes */
	u32	nr_sects;	  		/* How many sectors */
	u32	nr_inode_map_sects;	/* How many inode-map sectors */
	u32	nr_sect_map_sects;	/* How many sector-map sectors */
	u32	nr_1st_data_sect;	/* Sector number of the 1st data sector */
	u32	nr_inode_sects;   	/* How many inode sectors */
	u32	root_inode;       	/* Inode nr of root directory */
	u32	inode_size;       	/* INODE_SIZE */
	u32	inode_isize_off;  	/* Offset of 'inode::inode_file_size' */
	u32	inode_start_off;  	/* Offset of 'inode::inode_sect_start' */
	u32	dir_ent_size;     	/* DIR_ENTRY_SIZE */
	u32	dir_ent_inode_off;	/* Offset of 'dir_entry::inode_nr' */
	u32	dir_ent_fname_off;	/* Offset of 'dir_entry::name' */
	/* sb_dev is only present in memory */
	int	sb_dev; 			/* the super block's home device */
							
} super_block;

/* from inode_file_type to inode_nr_sects of inode, not include rest */
#define	INODE_SIZE			32
typedef struct{
	u32	inode_file_type;	/* Accsess mode */
	u32	inode_file_size;	/* File size */
	u32	inode_sect_start;	/* The first sector of the data */
	u32	inode_nr_sects;		/* How many sectors the file occupies */
	u8	_unused[16];		/* Stuff for alignment */
	/* the following items are only present in memory */
	int	inode_dev;			/* device nr*/
	int	inode_cnt;			/* How many procs share this inode */
	int	inode_nr;			/* inode nr. */
} inode;

typedef struct directory_entry {
	int		inode_nr;
	char	name[MAX_FILENAME_LEN];
} dir_entry;

typedef struct file_descriptor {
	int		fd_mode;	/* Read or Write */
	int		fd_pos;		/* Current position for R/W. */
	int		fd_cnt;		/* How many procs share this desc */
	inode*	fd_inode;	/* Poniter to the i-node */
} file_desc;

#define RD_SECT(dev_nr, start_sect_nr) rd_wr_sector(DEV_READ, \
												dev_nr,				\
												(start_sect_nr) * SECTOR_SIZE,		\
												SECTOR_SIZE, /* read one sector */ \
												TASK_FS,				\
												fsbuf);
#define WR_SECT(dev_nr, start_sect_nr) rd_wr_sector(DEV_WRITE, \
												dev_nr,				\
												(start_sect_nr) * SECTOR_SIZE,		\
												SECTOR_SIZE, /* write one sector */ \
												TASK_FS,				\
												fsbuf);
#define RD_SECTS(dev_nr, start_sect_nr, sect_size) rd_wr_sector(DEV_READ, \
												dev_nr,				\
												(start_sect_nr) * SECTOR_SIZE,		\
												(sect_size) * SECTOR_SIZE, /* read two or more sectors */ \
												TASK_FS,				\
												fsbuf);
#define WR_SECTS(dev_nr, start_sect_nr, sect_size) rd_wr_sector(DEV_WRITE, \
												dev_nr,				\
												(start_sect_nr) * SECTOR_SIZE,		\
												(sect_size) * SECTOR_SIZE, /* write two or more sectors */ \
												TASK_FS,				\
												fsbuf);
	
#endif /* _KOS_FS_H_ */
