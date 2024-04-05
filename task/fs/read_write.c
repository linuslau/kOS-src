#include "shared.h"

static int do_read_write_special(inode *p_inode, void *buf, int len, int caller_pid);
static int do_read_write_regular(inode *p_inode, void *buf, int len, int caller_pid, int fd);

/* No need to update sector map, since the sectors for the file are
 * allocated and the bits are set when the file was created. */
int do_read_write()
{
	int fd 			= fs_msg.FD;
	void * buf 		= fs_msg.BUF;
	int len			= fs_msg.CNT;
	int caller_pid 	= fs_msg.source;

	assert((caller_proc->file_desc[fd] >= &file_desc_table[0]) &&
	       (caller_proc->file_desc[fd] <  &file_desc_table[NR_FILE_DESC]));

	if (!(caller_proc->file_desc[fd]->fd_mode & O_RDWR))
		return 0;

	inode * p_inode = caller_proc->file_desc[fd]->fd_inode;
	assert(p_inode >= &inode_table[0] && p_inode < &inode_table[NR_INODE]);

	int file_type = p_inode->inode_file_type & TYPE_MASK;
	if (file_type == TYPE_SPECIAL_CHAR)
        return do_read_write_special(p_inode, buf, len, caller_pid);
    else
        return do_read_write_regular(p_inode, buf, len, caller_pid, fd);
}

int do_read_write_special(inode *p_inode, void *buf, int len, int caller_pid)
{
	int type = fs_msg.type == FS_READ ? DEV_READ : DEV_WRITE;
	fs_msg.type = type;

	int dev = p_inode->inode_sect_start;
	assert(GET_DEV_MAJOR_NR(dev) == 4);

	fs_msg.NR_DEV_MINOR	= GET_DEV_MINOR_NR(dev);
	fs_msg.BUF			= buf;
	fs_msg.CNT			= len;
	fs_msg.PROC_NR		= caller_pid;
	assert(dev_drv_map[GET_DEV_MAJOR_NR(dev)].driver_nr != INVALID_DRIVER);
	ipc_msg_send_rcv(SEND_RECEIVE, dev_drv_map[GET_DEV_MAJOR_NR(dev)].driver_nr, &fs_msg);
	assert(fs_msg.CNT == len);

	return fs_msg.CNT;
}

int do_read_write_regular(inode *p_inode, void *buf, int len, int caller_pid, int fd)
{
	assert(p_inode->inode_file_type == TYPE_REGULAR || p_inode->inode_file_type == TYPE_DIRECTORY);
	assert((fs_msg.type == FS_READ) || (fs_msg.type == FS_WRITE));

	int pos_start 		= caller_proc->file_desc[fd]->fd_pos;
	int pos_end;
	if (fs_msg.type == FS_READ)
		pos_end = min(pos_start + len, p_inode->inode_file_size);
	else /* FS_WRITE */
		pos_end = min(pos_start + len, p_inode->inode_nr_sects * SECTOR_SIZE);

	int pos_off		  	=  pos_start % SECTOR_SIZE;
	int rw_sect_min   	=  p_inode->inode_sect_start + (pos_start >> SECTOR_SIZE_SHIFT);
	int rw_sect_max   	=  p_inode->inode_sect_start + (pos_end >> SECTOR_SIZE_SHIFT);
	int rw_sect_chunk 	=  min(rw_sect_max - rw_sect_min + 1, FS_BUF_SIZE >> SECTOR_SIZE_SHIFT);

	int bytes_rw 	  	=  0;
	int bytes_left    	=  len;
	for (int i = rw_sect_min; i <= rw_sect_max; i += rw_sect_chunk) {
		/* read/write this amount of bytes every time */
		int bytes = min(bytes_left, rw_sect_chunk * SECTOR_SIZE - pos_off);
		RD_SECTS(p_inode->inode_dev, i, rw_sect_chunk);

		if (fs_msg.type == FS_READ) {
			phys_copy((void*)proc_va2la(caller_pid, buf + bytes_rw),
						(void*)proc_va2la(TASK_FS, fsbuf + pos_off),
						bytes);
		} else {						/* FS_WRITE */
			phys_copy((void*)proc_va2la(TASK_FS, fsbuf + pos_off),
						(void*)proc_va2la(caller_pid, buf + bytes_rw),
						bytes);
			WR_SECTS(p_inode->inode_dev, i, rw_sect_chunk);
		}
		pos_off = 0;
		bytes_rw += bytes;
		caller_proc->file_desc[fd]->fd_pos += bytes;
		bytes_left -= bytes;
	}

	if (caller_proc->file_desc[fd]->fd_pos > p_inode->inode_file_size) {
		p_inode->inode_file_size = caller_proc->file_desc[fd]->fd_pos;
		write_inode(p_inode);
	}

	return bytes_rw;
}