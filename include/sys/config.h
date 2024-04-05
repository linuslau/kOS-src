#define	INSTALL_START_SECT		0x8000
#define	INSTALL_NR_SECTS		0x800

#define	BOOT_PARAM_ADDR			0x900
#define	BOOT_PARAM_MAGIC		0xB007

#define	BP_IDX_MAGIC			0
#define	BP_IDX_RAM_SIZE			1
#define	BP_IDX_KERNEL_ADDR		2

/* Along with boot/include/bootloader.inc::ROOT_BASE, which should be changed if this macro is changed. */
#define	NR_MINOR_BOOT_PART      MINOR_NR_hd2a

/* disk log */
#define ENABLE_DISK_LOG
#define SET_LOG_SECT_SMAP_AT_STARTUP
#define MEMSET_LOG_SECTS
#define	NR_SECTS_FOR_LOG		NR_FILE_SECTS
