#include "shared.h"
#include "hd.h"

static 	void	hd_cmd_out				(struct cmd_block_reg* cmd);
static 	int		wait_for_status			(int mask, int val, int timeout);
static 	void	wait_for_hd_int			();
static 	void	hd_identify				(int drive);
static 	void	print_identify_info		(u16* hdinfo);
static 	void	get_part_table			(int drive, int sect_nr, part_entry * entry);
static 	void	read_partition			(int nr_dev_minor, int partition_type);
static 	void	print_partition			(struct hd_info * hdi);
static 	void 	read_primary_partitions	(int drive, struct hd_info *hdi, part_entry *part_tbl, int nr_dev_minor);
static 	void 	read_logical_partitions	(int drive, struct hd_info *hdi, part_entry *part_tbl, int nr_dev_minor);


static	u8							hd_status;
static	u8							hdbuf[SECTOR_SIZE * 2];
static	struct 						hd_info	hd_info[1];

#define	DRIVE_OF_DEV(nr_dev_minor) (nr_dev_minor <= NR_MAX_MINOR_PRIM ? \
		nr_dev_minor / NR_MAX_MINOR_PRIM_PER_DRIVE : (nr_dev_minor - MINOR_NR_hd1a) / NR_MAX_MINOR_LOGI_PER_DRIVE)

/* Ring 1 */
void hd_init()
{
	/* 0x475: Get total number of hard drives from the BIOS data area */
	u8 * pNrDrives = (u8*)(0x475);
	printl("HD:   NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);

	set_irq_handler	(IRQ_AT_WINI, hd_handler);
	enable_irq		(IRQ_CASCADE);
	enable_irq		(IRQ_AT_WINI);

	for (int i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	hd_info[0].open_cnt = 0;
}

/* Ring 1 */
void hd_open(int nr_dev_minor)
{
	int drive = DRIVE_OF_DEV(nr_dev_minor);
	assert(drive == 0);

	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0) {
		/* no matter what nr_dev_minor is, partition is always in hd0(0) or hd5(5)*/
		read_partition(drive * (NR_MAX_PRIM_PART_PER_DRIVE + 1), PRIMARY_PARTITION);
		print_partition(&hd_info[drive]);
	}
}

/* Ring 1 */
void hd_close(int nr_dev_minor)
{
	int drive = DRIVE_OF_DEV(nr_dev_minor);
	assert(drive == 0);
	hd_info[drive].open_cnt--;
}

/* Ring 1 */
void hd_read_write(message * p)
{
	struct cmd_block_reg cmd;
	int drive = DRIVE_OF_DEV(p->NR_DEV_MINOR);

	u64 pos = p->POSITION;
	assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));

	/* We only allow to R/W from a SECTOR boundary: */
	assert((pos & 0x1FF) == 0);

	u32 sect_nr 	= 	(u32)(pos >> SECTOR_SIZE_SHIFT);
	int logidx 		= 	(p->NR_DEV_MINOR - MINOR_NR_hd1a) % NR_MAX_MINOR_LOGI_PER_DRIVE;
	sect_nr 		+= 	p->NR_DEV_MINOR < NR_MAX_MINOR_PRIM ?
						hd_info[drive].primary[p->NR_DEV_MINOR].start_sect :
						hd_info[drive].logical[logidx].start_sect;

	cmd.features	= 0;
	cmd.count		= (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low		= sect_nr & 0xFF;
	cmd.lba_mid		= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	u8 	lba_highest = (sect_nr >> 24) & 0xF;
	cmd.device		= MAKE_DEVICE_REG(LBA_MODE, drive, lba_highest);
	cmd.command		= (p->type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out		(&cmd);

	int bytes_left  = p->CNT;
	void * fsbuf_la = (void*)proc_va2la(p->PROC_NR, p->BUF);

	while (bytes_left) {
		int bytes 	= min(SECTOR_SIZE, bytes_left);
		if (p->type == DEV_READ) {
			wait_for_hd_int();
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(fsbuf_la, (void*)proc_va2la(TASK_HD, hdbuf), bytes);
		} else { /* DEV_WRITE */
			if (!wait_for_status(STATUS_REG_BIT_DRQ, STATUS_REG_BIT_DRQ, HD_TIMEOUT))
				panic("hd writing error.");
			port_write(REG_DATA, fsbuf_la, bytes);
			wait_for_hd_int();
		}
		bytes_left 	-= SECTOR_SIZE;
		fsbuf_la 	+= SECTOR_SIZE;
	}
}															

/* Ring 1 */
void hd_ioctl(message * p)
{
	int device 	= p->NR_DEV_MINOR;
	int drive 	= DRIVE_OF_DEV(device);

	struct hd_info * hdi = &hd_info[drive];

	if (p->REQUEST == IOCTL_GET_PART) {
		void * dst = proc_va2la(p->PROC_NR, p->BUF);
		void * src = proc_va2la(TASK_HD,
								device < NR_MAX_MINOR_PRIM ?
								&hdi->primary[device] :
								&hdi->logical[(device - MINOR_NR_hd1a) % NR_MAX_MINOR_LOGI_PER_DRIVE]);

		phys_copy(dst, src, sizeof(part_info));
	}
	else {
		assert(0);
	}
}

/* Ring 1 */
static void get_part_table(int drive, int sect_nr, part_entry * entry)
{
	struct cmd_block_reg cmd;
	cmd.features	= 0;
	cmd.count		= 1;
	cmd.lba_low		= sect_nr & 0xFF;
	cmd.lba_mid		= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	u8 lba_highest 	= (sect_nr >> 24) & 0xF;
	cmd.device		= MAKE_DEVICE_REG(LBA_MODE, drive, lba_highest);
	cmd.command		= ATA_READ;
	hd_cmd_out(&cmd);
	wait_for_hd_int();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	memcpy(entry, hdbuf + PART_TABLE_OFFSET, sizeof(part_entry) * NR_MAX_PRIM_PART_PER_DRIVE);
}

/* Ring 1, Read the partition table(s) and fills the hd_info struct. */
static void read_partition(int nr_dev_minor, int partition_type)
{
	int drive = DRIVE_OF_DEV(nr_dev_minor);
	struct hd_info * hdi = &hd_info[drive];

	part_entry part_tbl[NR_MAX_MINOR_LOGI_PER_DRIVE];

	if (partition_type == PRIMARY_PARTITION) {
		read_primary_partitions(drive, hdi, part_tbl, nr_dev_minor);
	}
	else if (partition_type == EXTENDED_PARTITION) {
		read_logical_partitions(drive, hdi, part_tbl, nr_dev_minor);
	}
	else {
		assert(0);
	}
}

static void read_primary_partitions(int drive, struct hd_info *hdi, part_entry *part_tbl, int nr_dev_minor)
{
	get_part_table(drive, drive, part_tbl);

	int nr_prim_parts = 0;
	for (int i = 0; i < NR_MAX_PRIM_PART_PER_DRIVE; i++) { /* 0~3 */
		if (part_tbl[i].sys_id == NO_PART) 
			continue;

		nr_prim_parts++;
		int dev_nr = i + 1;		  /* 1~4 */
		hdi->primary[dev_nr].start_sect = part_tbl[i].start_sect;
		hdi->primary[dev_nr].sect_cnt 	= part_tbl[i].nr_sects;

		if (part_tbl[i].sys_id == EXT_PART) /* extended */
			read_partition(nr_dev_minor + dev_nr, EXTENDED_PARTITION);
	}
	assert(nr_prim_parts != 0);
}

static void read_logical_partitions(int drive, struct hd_info *hdi, part_entry *part_tbl, int nr_dev_minor)
{
	int nr_ext_part					= nr_dev_minor % NR_MAX_MINOR_PRIM_PER_DRIVE; /* 1~4 */
	int ext_part_start_sect 		= hdi->primary[nr_ext_part].start_sect;
	int nr_dev_minor_1st_logi_part 	= (nr_ext_part - 1) * NR_MAX_LOGI_PART_PER_EXT; /* 0/16/32/48 */

	for (int i = 0; i < NR_MAX_LOGI_PART_PER_EXT; i++) {
		int dev_nr = nr_dev_minor_1st_logi_part + i;/* 0~15/16~31/32~47/48~63 */

		get_part_table(drive, ext_part_start_sect, part_tbl);

		hdi->logical[dev_nr].start_sect = ext_part_start_sect + part_tbl[0].start_sect;
		hdi->logical[dev_nr].sect_cnt 	= part_tbl[0].nr_sects;

		ext_part_start_sect = ext_part_start_sect + part_tbl[1].start_sect;

		if (part_tbl[1].sys_id == NO_PART)
			break;
	}
}

/* Ring 1 */
static void print_partition(struct hd_info * hdi)
{
	printl("\nHD:   [Size in sector]:\n");
	for (int i = 0; i < NR_MAX_PRIM_PART_PER_DRIVE + 1; i++) {
		printl("HD:  %sPRIMARY_PART(%d): base %d(0x%x), size %d(0x%x)\n",
		       i == 0 ? " " : "   ",
		       i,
		       hdi->primary[i].start_sect,
		       hdi->primary[i].start_sect,
		       hdi->primary[i].sect_cnt,
		       hdi->primary[i].sect_cnt);
	}
	for (int i = 0; i < NR_MAX_MINOR_LOGI_PER_DRIVE; i++) {
		if (hdi->logical[i].sect_cnt == 0)
			continue;
		printl("HD:       "
		       "LOGICAL_PART(%d): base %d(0x%x), size %d(0x%x)\n",
		       i,
		       hdi->logical[i].start_sect,
		       hdi->logical[i].start_sect,
		       hdi->logical[i].sect_cnt,
		       hdi->logical[i].sect_cnt);
	}
}

/* Ring 1 */
static void hd_identify(int drive)
{
	struct cmd_block_reg cmd;
	u8 lba_highest 	= 0;
	cmd.device  	= MAKE_DEVICE_REG(CHS_MODE, drive, lba_highest);
	cmd.command 	= ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	wait_for_hd_int();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	print_identify_info((u16*)hdbuf);

	u16* hdinfo 	= (u16*)hdbuf;

	hd_info[drive].primary[0].start_sect = 0;
	/* Total Nr of User Addressable Sectors */
	hd_info[drive].primary[0].sect_cnt = ((int)hdinfo[61] << 16) + hdinfo[60];
}

/* Ring 1 */
static void print_identify_info(u16* hdinfo)
{
	char 	ascii_value[64];
	struct 	iden_info_ascii_fmt iden_info[] = {{10, 20, "HD SN"}, {27, 40, "HD Model"}};

	/* each ascii_value for identify return takes two bytes*/
	for (int i = 0; i < sizeof(iden_info)/sizeof(iden_info[0]); i++) {
		char * p = (char*)&hdinfo[iden_info[i].idx];
		int j = 0;
		for (j = 0; j < iden_info[i].len/2; j++) {
			ascii_value[j*2+1] = *p++;
			ascii_value[j*2] = *p++;
		}
		ascii_value[j*2] = 0;
		printl("HD:   %s: %s\n", iden_info[i].desc, ascii_value);
	}

	int capabilities = hdinfo[49];
	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	int cmd_sets = hdinfo[83];

	printl("HD:   LBA supported: %s\n", (capabilities & 0x0200) ? "Yes" : "No");
	printl("HD:   LBA48 supported: %s\n", (cmd_sets & 0x0400) ? "Yes" : "No");
	printl("HD:   HD  size: %dMB\n", sectors * 512 / 1000000);
}

/* Ring 1，Send a command to HD controller via Port IO. */
static void hd_cmd_out(struct cmd_block_reg* cmd)
{
	if (!wait_for_status(STATUS_REG_BIT_BSY, STATUS_REG_BIT_BSY_0, HD_TIMEOUT))
		panic("error: hd busy.");

	out_byte(REG_DEV_CTRL, 0);
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);
	out_byte(REG_CMD,      cmd->command);
}

/* Ring 1 */
static void wait_for_hd_int()
{
	message msg;
	ipc_msg_send_rcv(RECEIVE, INTERRUPT, &msg);
}

/* Ring 1 */
static int wait_for_status(int mask, int val, int timeout)
{
	int t = get_ticks();

	while(((get_ticks() - t) * 1000 / HZ) < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;

	return 0;
}

/* Ring 0 */
void hd_handler(int irq)
{
	hd_status = in_byte(REG_STATUS);
	hw_int_notification(TASK_HD);
}
