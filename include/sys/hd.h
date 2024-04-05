#ifndef	_KOS_HD_H_
#define	_KOS_HD_H_

void	hd_init			();
void	hd_open			(int nr_device_minor);
void	hd_close		(int nr_device_minor);
void	hd_read_write	(message * p);
void	hd_ioctl		(message * p);

#define	HD_TIMEOUT				10000	/* millisec */
#define	PART_TABLE_OFFSET		0x1BE	/* https://wiki.osdev.org/Partition_Table */
#define ATA_IDENTIFY			0xEC
#define ATA_READ				0x20
#define ATA_WRITE				0x30
#define ATA_FLUSH				0xE7
#define LBA_MODE 				1
#define CHS_MODE 				0

#define	MAKE_DEVICE_REG(lba,drv,lba_highest) 	(((lba) << 6) | ((drv) << 4) | (lba_highest & 0xF) | 0xA0)

/* I/O Ports used by hard disk controllers. */
/* slave disk not supported yet, all master registers below */

/* Command Block Registers */
/*	MACRO						PORT			DESCRIPTION							INPUT/OUTPUT	*/
#define REG_DATA				0x1F0			/*	Data							I/O		*/
#define REG_FEATURES			0x1F1			/*	Features						O		*/
#define REG_ERROR				0x1F1			/*	Error							I		*/
#define REG_NSECTOR				0x1F2			/*	Sector Count					I/O		*/
#define REG_LBA_LOW				0x1F3			/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID				0x1F4			/*	Cylinder Low  / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH			0x1F5			/*	Cylinder High / LBA Bits 16-23	I/O		*/
#define REG_DEVICE				0x1F6			/*	Drive | Head | LBA bits 24-27	I/O		*/
#define REG_STATUS				0x1F7			/*	Status							I		*/
#define REG_CMD					0x1F7			/*	Command							O		*/

#define	STATUS_REG_BIT_BSY		0x80
#define	STATUS_REG_BIT_BSY_0 	0x0
#define	STATUS_REG_BIT_DRDY		0x40
#define	STATUS_REG_BIT_DFSE		0x20
#define	STATUS_REG_BIT_DSC		0x10
#define	STATUS_REG_BIT_DRQ		0x08
#define	STATUS_REG_BIT_CORR		0x04
#define	STATUS_REG_BIT_IDX		0x02
#define	STATUS_REG_BIT_ERR		0x01

/* Control Block Registers */
/*		MACRO					PORT			DESCRIPTION							INPUT/OUTPUT	*/
#define REG_DEV_CTRL			0x3F6			/*	Device Control					O		*/
#define REG_ALT_STATUS			REG_DEV_CTRL	/*	Alternate Status				I		*/
#define REG_DRV_ADDR			0x3F7			/*	Drive Address					I		*/

#define MAX_IO_BYTES			256				/*	How many sectors does one IO can handle */
typedef struct partition_entry{
	u8 boot_ind;
	u8 start_head;
	u8 start_sector;
	u8 start_cyl;
	u8 sys_id;
	u8 end_head;
	u8 end_sector;
	u8 end_cyl;
	u32 start_sect;
	u32 nr_sects;
} part_entry;

struct cmd_block_reg {
    u8 features;      // Features register - holds additional features or parameters for the command
    u8 count;         // Count register - specifies the number of sectors to be transferred in the I/O operation
    u8 lba_low;       // Low byte of Logical Block Address (LBA) - the least significant bits of the block address
    u8 lba_mid;       // Middle byte of LBA - the middle bits of the block address
    u8 lba_high;      // High byte of LBA - the most significant bits of the block address
    u8 device;        // Device register - identifies the drive or device involved in the operation
    u8 command;       // Command register - specifies the operation to be performed by the device
};

typedef struct partition_info {
	u32	start_sect;	/* position in sector */
	u32	sect_cnt;	/* how many sectors in this partition */
} part_info;

/* main drive struct, one entry per drive */
struct hd_info
{
	int    		open_cnt;
	part_info	primary[NR_MAX_MINOR_PRIM_PER_DRIVE];
	part_info	logical[NR_MAX_MINOR_LOGI_PER_DRIVE];
};
struct iden_info_ascii_fmt {
	int		idx;
	int		len;
	char	* desc;
};

#endif /* _KOS_HD_H_ */
