#include "shared.h"

int get_file_inode_nr(char * path)
{
	char file_name[MAX_PATH];
	memset(file_name, 0, MAX_FILENAME_LEN);
	inode * root_dir_inode;
	if (get_file_name(path, file_name, &root_dir_inode) != 0)
		return INVALID_INODE;

	if (file_name[0] == 0)	/* path: "/" */
		return root_dir_inode->inode_nr;

	int root_dir_sect0_nr 	= root_dir_inode->inode_sect_start;
	int nr_root_dir_sects 	= (root_dir_inode->inode_file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries		= root_dir_inode->inode_file_size / DIR_ENTRY_SIZE;

	int de_idx = 0;
	dir_entry * p_de;
	for (int i = 0; i < nr_root_dir_sects; i++) {
		RD_SECT(root_dir_inode->inode_dev, root_dir_sect0_nr + i);
		p_de = (dir_entry *)fsbuf;
		for (int j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,p_de++) {
			if (memcmp(file_name, p_de->name, MAX_FILENAME_LEN) == 0)
				return p_de->inode_nr;
			if (++de_idx > nr_dir_entries)
				break;
		}
		if (de_idx > nr_dir_entries)
			break;
	}
	
	return INVALID_INODE;
}

int get_file_name(const char * full_path, char * file_name, inode** p_inode)
{
	const char * s 	= full_path;
	char * t 		= file_name;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {
		if (*s == '/')
			return -1;
		*t++ = *s++;
		if (t - file_name >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*p_inode = root_inode;

	return 0;
}

