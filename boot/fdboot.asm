org  07c00h
jmp short start
nop

%include	"bootloader.inc"

BS_OEMName      db 'KezhaoOS'   ; OEM String, must be 8 bytes
BPB_BytsPerSec  dw 512          ; Bytes per sector
BPB_SecPerClus  db 1            ; Sectors per cluster
BPB_RsvdSecCnt  dw 1            ; Reserved sectors count for Boot Record
BPB_NumFATs     db 2            ; Number of FATs
BPB_RootEntCnt  dw 224          ; Maximum number of entries in the root directory
BPB_TotSec16    dw 2880         ; Total logical sectors
BPB_Media       db 0xF0         ; Media descriptor
BPB_FATSz16     dw 9            ; Sectors per FAT
BPB_SecPerTrk   dw 18           ; Sectors per track
BPB_NumHeads    dw 2            ; Number of heads (surfaces)
BPB_HiddSec     dd 0            ; Hidden sectors count
BPB_TotSec32    dd 0            ; If wTotalSectorCount is 0, record sector count here
BS_DrvNum       db 0            ; Drive number for interrupt 13
BS_Reserved1    db 0            ; Unused
BS_BootSig      db 29h          ; Extended boot signature (29h)
BS_VolID        dd 0            ; Volume serial number
BS_VolLab       db 'KezhaoOS1.0'; Volume label, must be 11 bytes
BS_FileSysType  db 'FAT12   '   ; File system type, must be 8 bytes

start:
    mov sp, STACK_BOTTOM_BOOT   ; Initialize stack pointer

    call InitSegRegs        	; Initialize segment registers
    call ClearScreen         	; Clear the screen

    mov dh, 0
    call PrintStr            	; Print a string

    call ResetFloppy          	; Reset the floppy disk
    mov word [wSectorNo], 19  	; Set the sector number to 19

    mov dh, 1
    call PrintStr              	; Print another string

compare_files_in_root_dir:
	cmp word [wRootDirSize], 0  	; Check if the root directory is fully read
    jz no_loader_found          	; If fully read, no LOADER.BIN found
    dec word [wRootDirSize]     	; Decrement the root directory size

    mov ax, LOADER_SEG
    mov es, ax                   	; es <- LOADER_SEG
    mov bx, LOADER_OFFSET        	; bx <- LOADER_OFFSET, so es:bx = LOADER_SEG:LOADER_OFFSET
    mov ax, [wSectorNo]          	; ax <- Sector number in Root Directory
    mov cl, 1
    call ReadFDSector

    mov si, loader_name          	; ds:si -> "LOADER  BIN"
    mov di, LOADER_OFFSET        	; es:di -> LOADER_SEG:0100 = LOADER_SEG*10h+100
    cld
    mov dx, 16 						; 512/32 = 16, each item (directory entry) is 32 bytes, each sector has 16 items.

compare_files_in_a_sector:
    cmp dx, 0                       ; Loop counter control
    jz compare_next_sector          ; Jump to the next sector if one sector is fully read
    dec dx                          ; Jump to the next sector
    mov cx, 11                      ; Initialize counter for comparing file name

compare_file_name:
    cmp cx, 0                        ; Check if all characters are compared
    jz loader_found                  ; If compared 11 characters and all are equal, it's found
    dec cx                           ; Decrement the counter

    lodsb                            ; ds:si -> al
    cmp al, byte [es:di]			 ; es:di is read from FAT32, ds:si is set manually
    jz compare_next_byte
    jmp compare_next_file            ; If any character is different, this Directory Entry is not the LOADER.BIN we are looking for

compare_next_byte:
    inc di                            ; Move to the next byte in the current directory entry
    jmp compare_file_name             ; Continue looping

compare_next_file:
    and di, 0FFE0h                    ; else ┓ di &= E0 to point to the beginning of the current entry
    add di, 20h
    mov si, loader_name
    jmp compare_files_in_a_sector

compare_next_sector:
    add word [wSectorNo], 1           ; Move to the next sector in the root directory
    jmp compare_files_in_root_dir

loader_found:                          ; After finding LOADER.BIN, continue here
    mov ax, RootDirSectors             ; Load the total number of root directory sectors
    and di, 0FFE0h                     ; di -> Start of the current entry
    add di, 01Ah                       ; di -> First Sector
    mov cx, word [es:di]               ; Load the sector index of LOADER.BIN in FAT
    push cx                            ; Save the sector index in the stack

    add cx, ax                         ; Calculate the absolute sector index of LOADER.BIN
    add cx, DeltaSectorNo              ; After this line, cx becomes the starting sector of LOADER.BIN (0-based index)

    mov ax, LOADER_SEG
    mov es, ax                         ; es <- LOADER_SEG
    mov bx, LOADER_OFFSET              ; bx <- LOADER_OFFSET, so es:bx = LOADER_SEG:LOADER_OFFSET = LOADER_SEG * 10h + LOADER_OFFSET
    mov ax, cx                         ; ax <- Sector number
    jmp load_loader

load_loader:
    call PrintDot   			  ; Print dot for each sector

    mov cl, 1                     ; Set the sector read count to 1
    call ReadFDSector              ; Read the sector
    pop ax                        ; Retrieve the sector index in FAT from the stack

    call FAT12GetNextSector     ; Get the next sector index from FAT
    cmp ax, 0FFFh			      ; End token is 0FFF
    jz loader_loaded              ; If end token is reached, loader is loaded

    push ax                       ; Save the sector index in the stack
    mov dx, RootDirSectors        ; Load the total number of root directory sectors
    add ax, dx                    ; Add the total root directory sectors to the sector index
    add ax, DeltaSectorNo         ; Add the delta sector number
    add bx, [BPB_BytsPerSec]      ; Add the bytes per sector to the offset
    jmp load_loader               ; Continue loading the loader

loader_loaded:
    mov dh, 2                     ; Set dh to 2 ("Ready.")
    call PrintStr                ; Display the string
    call jump_to_loader           ; Jump to the loaded loader code

jump_to_loader:
    jmp LOADER_SEG:LOADER_OFFSET

no_loader_found:
    mov dh, 3
    call PrintStr
    jmp $

PrintDot:
    push ax
    push bx
    mov ah, 0Eh
    mov al, '.'
    mov bl, 0Fh
    int 10h
    pop bx
    pop ax
    ret

; dh refers to both line# and msg id
PrintStr:
    mov ax, msg_len
    mul dh
    add ax, msg_base
    mov bp, ax
    mov ax, ds
    mov es, ax
    mov cx, msg_len
    mov ax, 01301h
    mov bx, 0007h
    mov dl, 0
    int 10h
    ret

; Reads cl sectors starting from sector ax into es:bx
ReadFDSector:
	; Calculating the disk position from the sector number (Sector Number -> Cylinder Number, Starting Sector, Head Number)
	; -----------------------------------------------------------------------
	; Let the sector number be x
	;                           	  ┌ Cylinder Number = y >> 1
	;       x           ┌ Quotient y ┤
	; -------------- => ┤            └ Head Number = y & 1
	;  Sectors per Track│
	;                   └ Remainder z => Starting Sector Number = z + 1
	push bp
	mov bp, sp
	sub esp, 2                ; Allocate space for two bytes on the stack to store the number of sectors to be read: byte [bp-2]

	mov byte [bp-2], cl       ; Save the number of sectors to read
	push bx                   ; Save bx
	mov bl, [BPB_SecPerTrk]   ; bl: divisor
	div bl                    ; y in al, z in ah
	inc ah                    ; z ++
	mov cl, ah                ; cl <- Starting Sector Number
	mov dh, al                ; dh <- y
	shr al, 1                 ; y >> 1 (actually y/BPB_NumHeads, where BPB_NumHeads=2)
	mov ch, al                ; ch <- Cylinder Number
	and dh, 1                 ; dh & 1 = Head Number
	pop bx                    ; Restore bx
	;Now, got Cylinder Number, Starting Sector Number, Head Number.
	mov dl, [BS_DrvNum]       ; Drive Number (0 indicates A drive)
.read_loop:
	mov ah, 2                 ; Read
	mov al, byte [bp-2]       ; Read al sectors
	int 13h
	jc .read_loop           ; If there is a read error, CF is set to 1. Keep reading until successful.

	add esp, 2
	pop bp
	ret

;Finds the entry in the FAT corresponding to sector number ax and stores the result in ax.
;Note that it reads the FAT sector to es:bx, so the function saves es and bx at the beginning.
FAT12GetNextSector:
    push es                             ; Save es
    push bx                             ; Save bx
    push ax                             ; Save ax
    mov ax, LOADER_SEG                  ; ┓
    sub ax, 0100h                       ; ┣ Reserve 4K space behind LOADER_SEG for storing FAT
    mov es, ax                          ; ┛
    pop ax                              ; Restore ax
    mov byte [bOdd], 0
    mov bx, 3
    mul bx                              ; dx:ax = ax * 3
    mov bx, 2
    div bx                              ; dx:ax / 2  ==>  ax <- quotient, dx <- remainder
    cmp dx, 0
    jz even_num
    mov byte [bOdd], 1

even_num: ; Even number
    xor dx, dx                          ; Now ax contains the offset of the FAT entry in FAT. Next, calculate in which sector
    mov bx, [BPB_BytsPerSec]
    div bx                              ; dx:ax / BPB_BytsPerSec  ==>  ax <- quotient   (Sector number relative to FAT for the FAT entry)
                                        ;                        dx <- remainder (Offset of the FAT entry within the sector).
    push dx                             ; Save the remainder
    mov bx, 0                           ; bx <- 0   Therefore, es:bx = (LOADER_SEG - 100):00 = (LOADER_SEG - 100) * 10h
    add ax, FAT1SectorNo                ; After this line, ax is the sector number where the FAT entry is located
    mov cl, 2
    call ReadFDSector                    ; Read the sector containing the FAT entry, read two at a time to avoid errors at the boundary,
                                        ; because a FAT entry may span two sectors.
    pop dx                              ; Restore the remainder
    add bx, dx                          ; Adjust bx with the remainder
    mov ax, [es:bx]                     ; Load the FAT entry
    cmp byte [bOdd], 1
    jnz even_num_2
    shr ax, 4                           ; For odd entries, shift right by 4 bits

even_num_2:
    and ax, 0FFFh                       

    pop bx                              ; Restore bx
    pop es                              ; Restore es
    ret

InitSegRegs:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret

;Clears the screen with a black background and white text.
ClearScreen:
    mov ax, 0600h     ; AH = 6, AL = 0h
    mov bx, 0700h     ; Black background, white text (BL = 07h)
    mov cx, 0         ; Top-left corner: (0, 0)
    mov dx, 0184fh    ; Bottom-right corner: (80, 50)
    int 10h           ; BIOS interrupt for video services
    ret

;Resets the floppy drive.
ResetFloppy:
    xor ah, ah
    xor dl, dl
    int 13h
    ret

; Some constant definitions based on the FAT12 header. If the header information changes, these constants may need to be adjusted accordingly.
FAT1Sectors         equ 9
FAT2Sectors         equ 9
FAT1SectorNo        equ 1
RootDirSectors      equ 14
RootDirSectorNo     equ 19
DeltaSectorNo       equ 17

; Variables
wRootDirSize    dw 14     ; The number of sectors occupied by the Root Directory, decremented to zero in the loop.
wSectorNo       dw 0      ; The sector number to be read.
bOdd            db 0      ; Odd or even flag

; Strings
loader_name     db "FDLOADERBIN", 0
msg_base:       db "@MBR    "
msg2            db "Loading "
msg3            db "Ready.  "
msg4            db "NoLOADER"
msg_len         equ 8

times 510-($-$$) db 0; Fill the remaining space to ensure the generated binary code is exactly 512 bytes
dw 0xaa55            ; End flag