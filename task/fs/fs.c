#include "shared.h"
#include "config.h"
#include "hd.h"

static void get_partition_info	 	(part_info * pi);
static void create_super_block	 	(super_block * sb, part_info *pi);
static void create_inode_map	 	();
static void create_sector_map	 	(super_block * sb);
static void create_cmd_dot_tar 	 	(super_block * sb);
static void create_inode_array	 	(super_block * sb);
static void create_root_directory	(super_block * sb);

 /* Ring 1 */
void init_fs()
{
	for (int i = 0; i < NR_FILE_DESC; i++)
		memset(&file_desc_table[i], 0, sizeof(file_desc));

	for (int i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(inode));

	super_block * sb = super_block_table;
	for (; sb < &super_block_table[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NR_MAJOR_NO_DEV;

	message driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.NR_DEV_MINOR = GET_DEV_MINOR_NR(ROOT_DEV_NR);
	assert(dev_drv_map[GET_DEV_MAJOR_NR(ROOT_DEV_NR)].driver_nr != INVALID_DRIVER);
	ipc_msg_send_rcv(SEND_RECEIVE, dev_drv_map[GET_DEV_MAJOR_NR(ROOT_DEV_NR)].driver_nr, &driver_msg);

	RD_SECT(ROOT_DEV_NR, 1);
	sb = (super_block *)fsbuf;
	if (sb->magic != MAGIC_V1) {
		printl("FS:   Making new file system on disk\n");
		mkfs();
	}

	read_super_block    (ROOT_DEV_NR);
	sb = get_super_block(ROOT_DEV_NR);
	assert(sb->magic == MAGIC_V1);

	root_inode = get_file_inode(ROOT_DEV_NR, ROOT_INODE_NR);
}

void mkfs()
{
	part_info 	pi;
	super_block sb;
	get_partition_info(&pi);
	create_super_block(&sb, &pi);
	create_inode_map();
	create_sector_map(&sb);
	create_cmd_dot_tar(&sb);
	create_inode_array(&sb);
	create_root_directory(&sb);
}

void get_partition_info(part_info * part_info)
{
	message 				driver_msg;
	driver_msg.type			= DEV_IOCTL;
	driver_msg.NR_DEV_MINOR	= GET_DEV_MINOR_NR(ROOT_DEV_NR);
	driver_msg.REQUEST		= IOCTL_GET_PART;
	driver_msg.BUF			= part_info;
	driver_msg.PROC_NR		= TASK_FS;
	assert(dev_drv_map[GET_DEV_MAJOR_NR(ROOT_DEV_NR)].driver_nr != INVALID_DRIVER);

	ipc_msg_send_rcv(SEND_RECEIVE, dev_drv_map[GET_DEV_MAJOR_NR(ROOT_DEV_NR)].driver_nr, &driver_msg);
	printl("FS:   dev size: 0x%x sectors\n", part_info->sect_cnt);
}

void create_super_block(super_block *sb, part_info *pi)
{
	sb->magic	  			= MAGIC_V1;
	sb->nr_inodes	  		= SECTOR_BITS;
	sb->nr_inode_sects 		= sb->nr_inodes * INODE_SIZE / SECTOR_SIZE;
	sb->nr_sects	  		= pi->sect_cnt; /* partition size in sector */
	sb->nr_inode_map_sects  = 1;
	sb->nr_sect_map_sects  	= sb->nr_sects / SECTOR_BITS + 1;
	sb->nr_1st_data_sect	= 1 + 1 + sb->nr_inode_map_sects + sb->nr_sect_map_sects + sb->nr_inode_sects;
	sb->root_inode	  	 	= ROOT_INODE_NR;
	sb->inode_size	  	 	= INODE_SIZE;
	inode x;
	sb->inode_isize_off	 	= (int)&x.inode_file_size - (int)&x;
	sb->inode_start_off	 	= (int)&x.inode_sect_start - (int)&x;
	sb->dir_ent_size	  	= DIR_ENTRY_SIZE;
	dir_entry de;
	sb->dir_ent_inode_off 	= (int)&de.inode_nr - (int)&de;
	sb->dir_ent_fname_off 	= (int)&de.name - (int)&de;

	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, sb, 	SUPER_BLOCK_SIZE);
	WR_SECT(ROOT_DEV_NR, 1);

	printl("FS:   devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00\n"
	       "      inodes:0x%x00,  1st_sector:0x%x00\n",
	       pi->start_sect * 2,
	       (pi->start_sect + 1) * 2,
	       (pi->start_sect + 1 + 1) * 2,
	       (pi->start_sect + 1 + 1 + sb->nr_inode_map_sects) * 2,
	       (pi->start_sect + 1 + 1 + sb->nr_inode_map_sects + sb->nr_sect_map_sects) * 2,
	       (pi->start_sect + sb->nr_1st_data_sect) * 2);
}

void create_inode_map()
{
	memset(fsbuf, 0, SECTOR_SIZE);
	for (int i = 0; i < (NR_CONSOLES + 3); i++)
		fsbuf[0] |= 1 << i;

	assert(fsbuf[0] == 0x3F);
				 /* 0011 1111 :
				  *   || ||||
				  *   || |||'--- bit 0 : reserved
				  *   || ||'---- bit 1 : root dir '/'
				  *   || |'----- bit 2 : /dev_tty0
				  *   || '------ bit 3 : /dev_tty1
				  *   |'-------- bit 4 : /dev_tty2
				  *   '--------- bit 5 : /cmd.tar */
	WR_SECT(ROOT_DEV_NR, 2);
}

void create_sector_map(super_block *sb)
{
	int i, j;
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_FILE_SECTS + 1; /* 1: bit 0 is reserved for root dir '/' */
	for (i = 0; i < nr_sects / 8; i++)
		fsbuf[i] = 0xFF;

	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects);

	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb->nr_sect_map_sects; i++)
		WR_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects + i);
}

void create_cmd_dot_tar(super_block *sb)
{
	/* make sure it'll not be overwritten by the disk log */
	assert(INSTALL_START_SECT + INSTALL_NR_SECTS < sb->nr_sects - NR_SECTS_FOR_LOG);
	int bit_offset 		= INSTALL_START_SECT - sb->nr_1st_data_sect + 1; /* sect M <-> bit (M - sb->n_1stsect + 1) */
	int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
	int bit_left 		= INSTALL_NR_SECTS;
	int cur_sect 		= bit_offset / (SECTOR_SIZE * 8);
	RD_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects + cur_sect);
	while (bit_left) {
		int byte_off = bit_off_in_sect / 8;
		/* this line is ineffecient in a loop, but I don't care */
		fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
		bit_left--;
		bit_off_in_sect++;
		if (bit_off_in_sect == (SECTOR_SIZE * 8)) {
			WR_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects + cur_sect);
			cur_sect++;
			RD_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects + cur_sect);
			bit_off_in_sect = 0;
		}
	}
	WR_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects + cur_sect);
}

void create_inode_array(super_block *sb)
{
	/* inode of '/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	inode * p_inode = (inode*)fsbuf;
	p_inode->inode_file_type = TYPE_DIRECTORY;
	p_inode->inode_file_size = DIR_ENTRY_SIZE * 5; /* 5 files:
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  * `cmd.tar'
					  */
	p_inode->inode_sect_start = sb->nr_1st_data_sect;
	p_inode->inode_nr_sects = NR_FILE_SECTS;
	/* inode of '/dev_tty0~2' */
	for (int i = 0; i < NR_CONSOLES; i++) {
		p_inode 					= (inode*)(fsbuf + (INODE_SIZE * (i + 1)));
		p_inode->inode_file_type	= TYPE_SPECIAL_CHAR;
		p_inode->inode_file_size 	= 0;
		p_inode->inode_sect_start 	= MAKE_DEV_NR(NR_MAJOR_DEV_CHAR_TTY, i);
		p_inode->inode_nr_sects 	= 0;
	}
	/* inode of '/cmd.tar' */
	p_inode = (inode*)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
	p_inode->inode_file_type 	= TYPE_REGULAR;
	p_inode->inode_file_size  	= INSTALL_NR_SECTS * SECTOR_SIZE;
	p_inode->inode_sect_start 	= INSTALL_START_SECT;
	p_inode->inode_nr_sects 	= INSTALL_NR_SECTS;
	WR_SECT(ROOT_DEV_NR, 2 + sb->nr_inode_map_sects + sb->nr_sect_map_sects);

}

void create_root_directory(super_block *sb)
{
	int i;
	memset(fsbuf, 0, SECTOR_SIZE);
	dir_entry * p_de = (dir_entry *)fsbuf;

	p_de->inode_nr = 1;
	strcpy(p_de->name, ".");

	/* dir entries of '/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) {
		p_de++;
		p_de->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		sprintf(p_de->name, "dev_tty%d", i);
	}
	(++p_de)->inode_nr = NR_CONSOLES + 2;
	sprintf(p_de->name, "cmd.tar", i);
	WR_SECT(ROOT_DEV_NR, sb->nr_1st_data_sect);
}

int rd_wr_sector(int io_type, int dev_nr, u64 pos, int bytes, int proc_nr, void* buf)
{
	assert(dev_drv_map[GET_DEV_MAJOR_NR(dev_nr)].driver_nr != INVALID_DRIVER);

	message 				driver_msg;
	driver_msg.type			= io_type;
	driver_msg.NR_DEV_MINOR	= GET_DEV_MINOR_NR(dev_nr);
	driver_msg.POSITION		= pos;
	driver_msg.BUF			= buf;
	driver_msg.CNT			= bytes;
	driver_msg.PROC_NR		= proc_nr;

	ipc_msg_send_rcv(SEND_RECEIVE, dev_drv_map[GET_DEV_MAJOR_NR(dev_nr)].driver_nr, &driver_msg);

	return 0;
}

/* Ring 1, Read super block from the given device then write it into a free super_block[] slot. */
void read_super_block(int dev_nr)
{
	int slot_idx;
	RD_SECT(dev_nr, 1);

	for (slot_idx = 0; slot_idx < NR_SUPER_BLOCK; slot_idx++)
		if (super_block_table[slot_idx].sb_dev == NR_MAJOR_NO_DEV)
			break;

	if (slot_idx == NR_SUPER_BLOCK)
		panic("super_block slots used up");

	assert(slot_idx == 0); /* currently we use only the 1st slot */

	super_block * p_sb					= (super_block *)fsbuf;
	super_block_table[slot_idx]			= *p_sb;
	super_block_table[slot_idx].sb_dev 	= dev_nr;
}

/* Ring 1, get the super block from super_block[]. */
super_block * get_super_block(int dev_nr)
{
	super_block * sb = super_block_table;
	for (; sb < &super_block_table[NR_SUPER_BLOCK]; sb++)
		if (sb->sb_dev == dev_nr)
			return sb;

	panic("super block of devie %d not found.\n", dev_nr);
	return 0;
}

/* Ring 1, inode_table[] is a cache maintained to make things faster */
inode * get_file_inode(int dev_nr, int inode_nr)
{
	if (inode_nr == 0)
		return 0;

	inode * p = 0;
	inode * q = 0;
	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) {
		if (p->inode_cnt) {	/* not a free slot */
			if ((p->inode_dev == dev_nr) && (p->inode_nr == inode_nr)) { /* this node is cached*/
				p->inode_cnt++;
				return p;
			}
		} else {/* a free slot */
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		panic("the inode table is full");

	q->inode_dev = dev_nr;
	q->inode_nr  = inode_nr;
	q->inode_cnt = 1;

	super_block * sb = get_super_block(dev_nr);
	int sect_nr = 1 + 1 + sb->nr_inode_map_sects + sb->nr_sect_map_sects +
							((inode_nr - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(dev_nr, sect_nr);

	inode * p_inode = (inode*)((u8*)fsbuf +
							((inode_nr - 1 ) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE);

	q->inode_file_type 	= p_inode->inode_file_type;
	q->inode_file_size 	= p_inode->inode_file_size;
	q->inode_sect_start = p_inode->inode_sect_start;
	q->inode_nr_sects 	= p_inode->inode_nr_sects;

	return q;
}

/* Ring 1 */
void put_inode(inode * p_inode)
{
	assert(p_inode->inode_cnt > 0);
	p_inode->inode_cnt--;
}

/* Ring 1 */
void write_inode(inode * p)
{
	inode * p_inode;
	super_block * sb = get_super_block(p->inode_dev);
	int sect_nr = 1 + 1 + sb->nr_inode_map_sects + sb->nr_sect_map_sects +
						((p->inode_nr - 1) / (SECTOR_SIZE / INODE_SIZE));
	
	RD_SECT(p->inode_dev, sect_nr);
	p_inode = (inode*)((u8*)fsbuf +
				(((p->inode_nr - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE));
	p_inode->inode_file_type 	= p->inode_file_type;
	p_inode->inode_file_size 	= p->inode_file_size;
	p_inode->inode_sect_start  	= p->inode_sect_start;
	p_inode->inode_nr_sects 	= p->inode_nr_sects;
	WR_SECT(p->inode_dev, sect_nr);
}

int fs_fork()
{
	proc* child = &proc_table[fs_msg.PID];
	for (int i = 0; i < NR_FILES; i++) {
		if (child->file_desc[i]) {
			child->file_desc[i]->fd_cnt++;
			child->file_desc[i]->fd_inode->inode_cnt++;
		}
	}
	return 0;
}

int fs_exit()
{
	proc* p = &proc_table[fs_msg.PID];
	for (int i = 0; i < NR_FILES; i++) {
		if (p->file_desc[i]) {
			p->file_desc[i]->fd_inode->inode_cnt--;
			if (--p->file_desc[i]->fd_cnt == 0)
				p->file_desc[i]->fd_inode = 0;
			p->file_desc[i] = 0;
		}
	}
	return 0;
}

