 * Master Boot Record (MBR): https://wiki.osdev.org/MBR_(x86)
 *   Located at offset 0x1BE in the 1st sector of a disk. MBR contains
 *   four 16-byte partition entries. Should end with 55h & AAh.
 *
 * Partitions in MBR: https://wiki.osdev.org/Partition_Table
 *   A PC hard disk can contain either as many as four primary partitions,
 *   or 1-3 primaries and a single extended partition. Each of these
 *   partitions are described by a 16-byte entry in the Partition Table
 *   which is located in the Master Boot Record.
 *
 * Extented partition: http://en.wikipedia.org/wiki/Extended_boot_record for details.

 * Each of the four Partition Table entries contains the following elements, in the following structure:

 * Offset	Size	Description
 * 0x00		1 byte	Boot indicator bit flag: 0 = no, 0x80 = bootable (or "active")
 * 0x01		1 byte	Starting head
 * 0x02		6 bits	Starting sector (Bits 6-7 are the upper two bits for the Starting Cylinder field.)
 * 0x03		10 bits	Starting Cylinder
 * 0x04		1 byte	System ID
 * 0x05		1 byte	Ending Head
 * 0x06		6 bits	Ending Sector (Bits 6-7 are the upper two bits for the ending cylinder field)
 * 0x07		10 bits	Ending Cylinder
 * 0x08		4 bytes	Relative Sector (to start of partition -- also equals the partition's starting LBA value)
 * 0x0C		4 bytes	Total Sectors in partition

/* 	The contents of this register are valid only when the error bit
    (ERR) in the Status Register is set, except at drive power-up or at the
    completion of the drive's internal diagnostics, when the register
    contains a status code.
    When the error bit (ERR) is set, Error Register bits are interpreted as such:
    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    +-----+-----+-----+-----+-----+-----+-----+-----+
    | BRK | UNC |     | IDNF|     | ABRT|TKONF| AMNF|
    +-----+-----+-----+-----+-----+-----+-----+-----+
        |     |     |     |     |     |     |     |
        |     |     |     |     |     |     |     `--- 0. Data address mark not found after correct ID field found
        |     |     |     |     |     |     `--------- 1. Track 0 not found during execution of Recalibrate command
        |     |     |     |     |     `--------------- 2. Command aborted due to drive status error or invalid command
        |     |     |     |     `--------------------- 3. Not used
        |     |     |     `--------------------------- 4. Requested sector's ID field not found
        |     |     `--------------------------------- 5. Not used
        |     `--------------------------------------- 6. Uncorrectable data error encountered
        `--------------------------------------------- 7. Bad block mark detected in the requested sector's ID field
*/

/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    +-----+-----+-----+-----+-----+-----+-----+-----+
    |  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
    +-----+-----+-----+-----+-----+-----+-----+-----+
                |           |   \_____________________/
                |           |              |
                |           |              `------------ If L=0, Head Select.
                |           |                                    These four bits select the head number.
                |           |                                    HS0 is the least significant.
                |           |                            If L=1, HS0 through HS3 contain bit 24-27 of the LBA.
                |           `--------------------------- Drive. When DRV=0, drive 0 (master) is selected. 
                |                                               When DRV=1, drive 1 (slave) is selected.
                `--------------------------------------- LBA mode. This bit selects the mode of operation.
                                                                When L=0, addressing is by 'CHS' mode.
                                                                When L=1, addressing is by 'LBA' mode.
*/

/* 	Any pending interrupt is cleared whenever this register is read.
    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    +-----+-----+-----+-----+-----+-----+-----+-----+
    | BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
    +-----+-----+-----+-----+-----+-----+-----+-----+
        |     |     |     |     |     |     |     |
        |     |     |     |     |     |     |     `--- 0. Error.(an error occurred)
        |     |     |     |     |     |     `--------- 1. Obsolete.
        |     |     |     |     |     `--------------- 2. Obsolete.
        |     |     |     |     `--------------------- 3. Data Request. (ready to transfer data)
        |     |     |     `--------------------------- 4. Command dependent. (formerly DSC bit)
        |     |     `--------------------------------- 5. Device Fault / Stream Error.
        |     `--------------------------------------- 6. Drive Ready.
        `--------------------------------------------- 7. Busy. If BSY=1, no other bits in the register are valid.
*/

/*
+--------+---------------------------------+-----------------+
| Command| Command Description             | Parameters Used |
| Code   |                                 | PC SC SN CY DH  |
+--------+---------------------------------+-----------------+
| ECh  @ | Identify Drive                  |             D   |
| 91h    | Initialize Drive Parameters     |    V        V   |
| 20h    | Read Sectors With Retry         |    V  V  V  V   |
| E8h  @ | Write Buffer                    |             D   |
+--------+---------------------------------+-----------------+

KEY FOR SYMBOLS IN THE TABLE:
===========================================-----=========================================================================
PC    Register 1F1: Write Precompensation	@     These commands are optional and may not be supported by some drives.
SC    Register 1F2: Sector Count		D     Only DRIVE parameter is valid, HEAD parameter is ignored.
SN    Register 1F3: Sector Number		D+    Both drives execute this command regardless of the DRIVE parameter.
CY    Register 1F4+1F5: Cylinder low + high	V     Indicates that the register contains a valid paramterer.
DH    Register 1F6: Drive / Head
*/

/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
+-----+-----+-----+-----+-----+-----+-----+-----+
| HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
+-----+-----+-----+-----+-----+-----+-----+-----+
    |                             |     |
    |                             |     `--------- Interrupt Enable.
    |                             |                  - IEN=0, and the drive is selected,
    |                             |                    drive interrupts to the host will be enabled.
    |                             |                  - IEN=1, or the drive is not selected,
    |                             |                    drive interrupts to the host will be disabled.
    |                             `--------------- Software Reset.
    |                                                - The drive is held reset when RST=1.
    |                                                  Setting RST=0 re-enables the drive.
    |                                                - The host must set RST=1 and wait for at least
    |                                                  5 microsecondsbefore setting RST=0, to ensure
    |                                                  that the drive recognizes the reset.
    `--------------------------------------------- HOB (High Order Byte)
                                                    - defined by 48-bit Address feature set.
*/