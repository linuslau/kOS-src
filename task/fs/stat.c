#include "shared.h"

int do_stat()
{
	char pathname[MAX_PATH];
	char filename[MAX_PATH];

	int name_len 	= fs_msg.NAME_LEN;
	int caller_pid 	= fs_msg.source;
	assert(name_len < MAX_PATH);
	phys_copy((void*)proc_va2la(TASK_FS, pathname),
			  (void*)proc_va2la(caller_pid, fs_msg.PATHNAME),
			  name_len);
	pathname[name_len] = 0;

	int inode_nr = get_file_inode_nr(pathname);
	if (inode_nr == INVALID_INODE) {
		printl("FS:   FS: :do_stat():: get_file_inode_nr() returns invalid inode: %s\n", pathname);
		return -1;
	}

	inode * p_inode = 0;
	inode * p_root_dir_inode;
	if (get_file_name(pathname, filename, &p_root_dir_inode) != 0)
		return -1;

	p_inode = get_file_inode(p_root_dir_inode->inode_dev, inode_nr);

	struct stat st;
	st.st_dev_nr  	= p_inode->inode_dev;
	st.st_inode_nr  = p_inode->inode_nr;
	st.st_file_mode = p_inode->inode_file_type;
	st.st_dev_nr_s 	= is_special(p_inode->inode_file_type) ? p_inode->inode_sect_start : NR_MAJOR_NO_DEV;
	st.st_file_size = p_inode->inode_file_size;

	put_inode(p_inode);

	phys_copy((void*)proc_va2la(caller_pid, fs_msg.BUF),
			  (void*)proc_va2la(TASK_FS, &st),
			  sizeof(struct stat));

	return 0;
}