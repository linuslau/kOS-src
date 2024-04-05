#include "shared.h"

static void clear_inode_map_bit		(inode *p_inode, int inode_nr);
static void clear_sector_map_bits	(inode *p_inode);
static void clear_inode_fields	 	(inode *p_inode);
static void clear_directory_entry	(inode *p_root_dir_inode, int inode_nr);

/*Note: do_unlink is not fully tested after code refactor */
/* Remove a file, clear the bit_idx-node in inode_array[]. Data bytes are not cleared */
int do_unlink() {
	char pathname[MAX_PATH];

	int name_len 		= fs_msg.NAME_LEN;
	int caller_pid 		= fs_msg.source;
	assert(name_len < MAX_PATH);
	phys_copy((void*)proc_va2la(TASK_FS, pathname),
			  (void*)proc_va2la(caller_pid, fs_msg.PATHNAME),
			  name_len);
	pathname[name_len] 	= 0;

	if (strcmp(pathname , "/") == 0) {
		printl("FS:   FS: do_unlink():: cannot unlink the root\n");
		return -1;
	}

	int inode_nr = get_file_inode_nr(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		printl("FS:   FS: :do_unlink():: get_file_inode_nr() returns invalid inode: %sect_nr\n", pathname);
		return -1;
	}

	char filename[MAX_PATH];
	inode * p_root_dir_inode;
	if (get_file_name(pathname, filename, &p_root_dir_inode) != 0)
		return -1;

	inode * p_inode = get_file_inode(p_root_dir_inode->inode_dev, inode_nr);

	if (p_inode->inode_file_type != TYPE_REGULAR) {
		printl("FS:   cannot remove file %sect_nr, because it is not a regular file.\n", pathname);
		return -1;
	}

	if (p_inode->inode_cnt > 1) {
		printl("FS:   cannot remove file %sect_nr, because p_inode->inode_cnt is %d.\n", pathname, p_inode->inode_cnt);
		return -1;
	}

    clear_inode_map_bit	 (p_inode, inode_nr);
    clear_sector_map_bits(p_inode);
    clear_inode_fields   (p_inode);
    clear_directory_entry(p_root_dir_inode, inode_nr);

    return 0;
}

/* clear the bit in inode-map */
void clear_inode_map_bit(inode *p_inode, int inode_nr) {
	int inode_byte_idx = inode_nr / 8;
	int inode_bit_idx  = inode_nr % 8;
	assert(inode_byte_idx < SECTOR_SIZE);	/* we have only one inode_bit_idx-map sector */
	RD_SECT(p_inode->inode_dev, 2); /* read sector 2 (skip bootsect and superblk): */
	assert(fsbuf[inode_byte_idx % SECTOR_SIZE] & (1 << inode_bit_idx));
	fsbuf[inode_byte_idx % SECTOR_SIZE] &= ~(1 << inode_bit_idx);
	WR_SECT(p_inode->inode_dev, 2);
}

/* clear the bits in sector-map */
void clear_sector_map_bits(inode *p_inode) {
    super_block *sb = get_super_block(p_inode->inode_dev);
    int start_bit_idx	= p_inode->inode_sect_start - sb->nr_1st_data_sect + 1;
	int start_byte_idx  = start_bit_idx / 8;
	int bits_occupied 	= p_inode->inode_nr_sects;
	int byte_cnt  		= (bits_occupied - (8 - (start_bit_idx % 8))) / 8;

	/* current sector nr. 2 = 1 + 1 = bootsect + superblk */
	int sect_nr = 2 + sb->nr_inode_map_sects + start_byte_idx / SECTOR_SIZE;
	RD_SECT(p_inode->inode_dev, sect_nr);

	int bit_idx;
	/* clear the first byte */
	for (bit_idx = start_bit_idx % 8; (bit_idx < 8) && bits_occupied; bit_idx++,bits_occupied--) {
		assert((fsbuf[start_byte_idx % SECTOR_SIZE] >> bit_idx & 1) == 1);
		fsbuf[start_byte_idx % SECTOR_SIZE] &= ~(1 << bit_idx);
	}

	/* clear bytes from the second byte to the second to the last */
	bit_idx = (start_byte_idx % SECTOR_SIZE) + 1;	/* the second byte */
	for (int i = 0; i < byte_cnt; i++,bit_idx++,bits_occupied-=8) {
		if (bit_idx == SECTOR_SIZE) {
			bit_idx = 0;
			WR_SECT(p_inode->inode_dev, sect_nr);
			RD_SECT(p_inode->inode_dev, ++sect_nr);
		}
		assert(fsbuf[bit_idx] == 0xFF);
		fsbuf[bit_idx] = 0;
	}

	/* clear the last byte */
	if (bit_idx == SECTOR_SIZE) {
		bit_idx = 0;
		WR_SECT(p_inode->inode_dev, sect_nr);
		RD_SECT(p_inode->inode_dev, ++sect_nr);
	}

	unsigned char mask = ~((unsigned char)(~0) << bits_occupied);
	assert((fsbuf[bit_idx] & mask) == mask);
	fsbuf[bit_idx] &= (~0) << bits_occupied;
	WR_SECT(p_inode->inode_dev, sect_nr);
}

/* Clear inode fields */
void clear_inode_fields(inode *p_inode) {
	p_inode->inode_file_type 	= 0;
	p_inode->inode_file_size 	= 0;
	p_inode->inode_sect_start	= 0;
	p_inode->inode_nr_sects	= 0;
	write_inode(p_inode);
	put_inode(p_inode);
}

/* Clear directory entry */
void clear_directory_entry(inode *p_root_dir_inode, int inode_nr) {
    int root_dir_sect0_nr = p_root_dir_inode->inode_sect_start;
	int nr_root_dir_sects = (p_root_dir_inode->inode_file_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries 	  = p_root_dir_inode->inode_file_size / DIR_ENTRY_SIZE;

	int de_nr 		 	= 0;
	int de_found 	 	= 0;
	int de_size 		= 0;
	dir_entry * p_de	= 0;

	for (int i = 0; i < nr_root_dir_sects; i++) {
		RD_SECT(p_root_dir_inode->inode_dev, root_dir_sect0_nr + i);

		p_de = (dir_entry *)fsbuf;
		for (int j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,p_de++) {
			if (++de_nr > nr_dir_entries)
				break;

			if (p_de->inode_nr == inode_nr) {
				memset(p_de, 0, DIR_ENTRY_SIZE);
				WR_SECT(p_root_dir_inode->inode_dev, root_dir_sect0_nr + i);
				de_found = 1;
				break;
			}

			if (p_de->inode_nr != INVALID_INODE)
				de_size += DIR_ENTRY_SIZE;
		}

		if (de_nr > nr_dir_entries || de_found)
			break;
	}
	assert(de_found);
	if (de_nr == nr_dir_entries) { /* the file is the last one in the dir */
		p_root_dir_inode->inode_file_size = de_size;
		write_inode(p_root_dir_inode);
	}
}
