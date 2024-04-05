#include "shared.h"

static inode * 	create_file			(char * full_path, int flags);
static int 		alloc_inode_map_bit	(int dev);
static int 		alloc_sect_map_bit	(int dev, int nr_sects_to_alloc);
static inode * 	make_new_inode		(int dev, int inode_nr, int start_sect);
static void 	make_new_dir_entry	(inode * root_dir_inode, int inode_nr, char * filename);

int do_open()
{
	int	 fd = -1;
	char pathname[MAX_PATH];

	int flags		= fs_msg.FLAGS;			/* access mode */
	int name_len 	= fs_msg.NAME_LEN;		/* length of filename */
	int pid 		= fs_msg.source;		/* caller proc nr. */
	assert(name_len < MAX_PATH);
	phys_copy((void*)proc_va2la(TASK_FS, pathname),
			  (void*)proc_va2la(pid, fs_msg.PATHNAME),
			  name_len);
	pathname[name_len] = 0;

	/* find a free slot in PROCESS::file_desc[] */
	for (int i = 0; i < NR_FILES; i++) {
		if (caller_proc->file_desc[i] == 0) {
			fd = i;
			break;
		}
	}
	if ((fd < 0) || (fd >= NR_FILES))
		panic("file_desc[] is full (PID:%d)", get_proc_pid(caller_proc));

	int inode_nr = get_file_inode_nr(pathname);

	inode * p_inode = 0;

	if (inode_nr == INVALID_INODE) { /* file not exist */
		if (flags & O_CREAT) {
			p_inode = create_file(pathname, flags);
		} else {
			printl("FS:   File does not exist: %s\n", pathname);
			return -1;
		}
	} else if (flags & O_RDWR) { /* file exists */
		if ((flags & O_CREAT) && (!(flags & O_TRUNC))) {
			assert(flags == (O_RDWR | O_CREAT));
			printl("FS:   file exists: %s\n", pathname);
			return -1;
		}
		assert((flags ==  O_RDWR                     ) ||
		       (flags == (O_RDWR | O_TRUNC          )) ||
		       (flags == (O_RDWR | O_TRUNC | O_CREAT)));

		char filename[MAX_PATH];
		inode * root_dir_inode;
		if (get_file_name(pathname, filename, &root_dir_inode) != 0)
			return -1;
		p_inode = get_file_inode(root_dir_inode->inode_dev, inode_nr);
	} else { /* file exists, no O_RDWR flag */
		printl("FS:   file exists: %s\n", pathname);
		return -1;
	}

	if (flags & O_TRUNC) {
		assert(p_inode);
		p_inode->inode_file_size = 0;
		write_inode(p_inode);
	}

	/* find a free slot in file_desc_table[] */
	int fdt_idx = 0;
	for (; fdt_idx < NR_FILE_DESC; fdt_idx++)
		if (file_desc_table[fdt_idx].fd_inode == 0)
			break;
	if (fdt_idx >= NR_FILE_DESC)
		panic("file_desc_table[] is full (PID:%d)", get_proc_pid(caller_proc));

	if (p_inode) {
		/* connects proc with file_descriptor */
		caller_proc->file_desc[fd] = &file_desc_table[fdt_idx];

		/* connects file_descriptor with inode */
		file_desc_table[fdt_idx].fd_inode = p_inode;
		file_desc_table[fdt_idx].fd_mode = flags;
		file_desc_table[fdt_idx].fd_cnt = 1;
		file_desc_table[fdt_idx].fd_pos = 0;

		int imode = p_inode->inode_file_type & TYPE_MASK;

		if (imode == TYPE_SPECIAL_CHAR) {
			message driver_msg;
			driver_msg.type = DEV_OPEN;
			int dev = p_inode->inode_sect_start;
			driver_msg.NR_DEV_MINOR = GET_DEV_MINOR_NR(dev);
			assert(GET_DEV_MAJOR_NR(dev) == 4);
			assert(dev_drv_map[GET_DEV_MAJOR_NR(dev)].driver_nr != INVALID_DRIVER);
			ipc_msg_send_rcv(SEND_RECEIVE, dev_drv_map[GET_DEV_MAJOR_NR(dev)].driver_nr, &driver_msg);
		} else if (imode == TYPE_DIRECTORY) {
			assert(p_inode->inode_nr == ROOT_INODE_NR);
		} else {
			assert(p_inode->inode_file_type == TYPE_REGULAR);
		}
	} else {
		return -1;
	}

	return fd;
}

int do_close()
{
	int fd = fs_msg.FD;
	put_inode(caller_proc->file_desc[fd]->fd_inode);
	if (--caller_proc->file_desc[fd]->fd_cnt == 0)
		caller_proc->file_desc[fd]->fd_inode = 0;
	caller_proc->file_desc[fd] = 0;

	return 0;
}

int do_lseek()
{
	int fd = fs_msg.FD;
	int off = fs_msg.OFFSET;
	int whence = fs_msg.WHENCE;

	int pos = caller_proc->file_desc[fd]->fd_pos;
	int f_size = caller_proc->file_desc[fd]->fd_inode->inode_file_size;

	switch (whence) {
		case SEEK_SET:
			pos = off;
			break;
		case SEEK_CUR:
			pos += off;
			break;
		case SEEK_END:
			pos = f_size + off;
			break;
		default:
			return -1;
			break;
	}
	if ((pos > f_size) || (pos < 0)) {
		return -1;
	}
	caller_proc->file_desc[fd]->fd_pos = pos;
	/* The new offset in bytes from the beginning of the file */
	return pos;
}

static inode * create_file(char * full_path, int flags)
{
	char filename[MAX_PATH];
	inode * root_dir_inode;
	if (get_file_name(full_path, filename, &root_dir_inode) != 0)
		return 0;

	int alloc_inode_nr 		= alloc_inode_map_bit(root_dir_inode->inode_dev);
	int alloc_sect_nr 		= alloc_sect_map_bit(root_dir_inode->inode_dev, NR_FILE_SECTS);
	inode *new_inode = make_new_inode(root_dir_inode->inode_dev, alloc_inode_nr, alloc_sect_nr);
	make_new_dir_entry(root_dir_inode, new_inode->inode_nr, filename);

	return new_inode;
}

/* @return  i-node nr */
static int alloc_inode_map_bit(int dev)
{
	int alloc_inode_nr = 0;

	int inode_map_start_sect = 1 + 1; /* 1 boot sector, 1 super block */
	super_block * sb = get_super_block(dev);

	for (int i = 0; i < sb->nr_inode_map_sects; i++) {
		RD_SECT(dev, inode_map_start_sect + i);

		for (int j = 0; j < SECTOR_SIZE; j++) {
			if (fsbuf[j] == 0xFF) /* this byte is 11111111 */
				continue;

			int k;
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) { /* find the last bit 1 in this byte */
			}

			alloc_inode_nr = (i * SECTOR_SIZE + j) * 8 + k; /* i: sector index; j: byte index; k: bit index */

			fsbuf[j] |= (1 << k);
			WR_SECT(dev, inode_map_start_sect + i);
			break;
		}

		return alloc_inode_nr;
	}

	panic("inode-map is probably full.\n");
	return 0;
}

/* @return  The 1st sector nr allocated. */
static int alloc_sect_map_bit(int dev, int nr_sects_to_alloc)
{
	super_block * sb = get_super_block(dev);

	int sect_map_sec_nr = 1 + 1 + sb->nr_inode_map_sects;
	int alloc_sect_nr = 0;

	for (int i = 0; i < sb->nr_sect_map_sects; i++) {
		RD_SECT(dev, sect_map_sec_nr + i);

		for (int j = 0; j < SECTOR_SIZE && nr_sects_to_alloc > 0; j++) {
			int k = 0;
			if (!alloc_sect_nr) {
				if (fsbuf[j] == 0xFF)
					continue;

				for (; ((fsbuf[j] >> k) & 1) != 0; k++) {
				}

				/* 1st sector is reserved for root directory, so -1, then + sb->nr_1st_data_sect */
				alloc_sect_nr = (i * SECTOR_SIZE + j) * 8 + k - 1 + sb->nr_1st_data_sect;
			}

			for (; k < 8; k++) { /* repeat till enough bits are set */
				assert(((fsbuf[j] >> k) & 1) == 0);
				fsbuf[j] |= (1 << k);
				if (--nr_sects_to_alloc == 0)
					break;
			}
		}

		if (alloc_sect_nr)
			WR_SECT(dev, sect_map_sec_nr + i);

		if (nr_sects_to_alloc == 0)
			break;
	}

	assert(nr_sects_to_alloc == 0);

	return alloc_sect_nr;
}

static inode * make_new_inode(int dev, int inode_nr, int start_sect)
{
	inode * new_inode = get_file_inode(dev, inode_nr);

	new_inode->inode_file_type 	= TYPE_REGULAR;
	new_inode->inode_file_size 	= 0;
	new_inode->inode_sect_start = start_sect;
	new_inode->inode_nr_sects 	= NR_FILE_SECTS;

	new_inode->inode_dev = dev;
	new_inode->inode_cnt = 1;
	new_inode->inode_nr	 = inode_nr;

	write_inode(new_inode);

	return new_inode;
}

static void make_new_dir_entry(inode *root_dir_inode,int inode_nr,char *filename)
{
	int root_dir_sect0_nr 	= root_dir_inode->inode_sect_start;
	int nr_root_dir_sects 	= (root_dir_inode->inode_file_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries		= root_dir_inode->inode_file_size / DIR_ENTRY_SIZE;

	dir_entry * p_de;
	dir_entry * p_de_new = 0;
	int sect_idx = 0;
	int de_idx = 0;

	for (; sect_idx < nr_root_dir_sects; sect_idx++) {
		RD_SECT(root_dir_inode->inode_dev, root_dir_sect0_nr + sect_idx);

		p_de = (dir_entry *)fsbuf;
		for (int j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,p_de++) {
			if (++de_idx > nr_dir_entries)
				break;

			if (p_de->inode_nr == 0) { /* found a free slot used before and file deleted*/
				p_de_new = p_de;
				break;
			}
		}
		if (p_de_new || de_idx > nr_dir_entries)
			break;
	}

	if (!p_de_new) {
		p_de_new = p_de; /* p_de reaches the end of the root dir */
		root_dir_inode->inode_file_size += DIR_ENTRY_SIZE;
	}

	p_de_new->inode_nr = inode_nr;
	strcpy(p_de_new->name, filename);

	WR_SECT(root_dir_inode->inode_dev, root_dir_sect0_nr + sect_idx);
	write_inode(root_dir_inode);
}
