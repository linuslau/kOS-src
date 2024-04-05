org  0x7c00
jmp short start

%include		"bootloader.inc"

disk_addr_packet:
	db	0x10				; [ 0] disk_addr_packet size in bytes.
	db	0					; [ 1] Reserved, must be 0.
	db	TRANSFER_SECT_NR	; [ 2] Nr of blocks to read/write.
	db	0					; [ 3] Reserved, must be 0.
	dw	0					; [ 4] Dst Addr of transfer - Seg:Offset  (Offset)
	dw	0					; [ 6] Dst Addr of transfer - Seg:Offset  (Seg)
	dd	0					; [ 8] LBA. Low  32-bits.
	dd	0					; [12] LBA. High 32-bits.

start:
	mov	sp, STACK_BOTTOM_BOOT
	call	InitSegRegs
	call	ClearScreen

	mov		dh, 0
	call	PrintStr

	; read the super block to SUPER_BLK_SEG::0
	mov	dword [disk_addr_packet +  6], SUPER_BLK_SEG
	; per sector, ROOT_BASE is boot sector, +1 is super block
	mov	dword [disk_addr_packet +  8], ROOT_BASE + 1
	; meanwhile, es:bx points to the same place
	call	ReadHDSector

	; super block value will be always be got from fs:xx
	mov	ax, SUPER_BLK_SEG
	mov	fs, ax

	; there is a ReadHDSector call in ReadINode,
	; configure the right output addr to avoid overwrite SUPER_BLK_SEG::0
	mov	dword [disk_addr_packet +  4], LOADER_OFFSET
	mov	dword [disk_addr_packet +  6], LOADER_SEG

	; get Inode nr of root directory (root directory is also a file)
	mov	eax, [fs:SB_ROOT_INODE_NR]
	; get the sector nr of '/' (ROOT_INODE_NR),
	; it'll be stored in eax
	call	ReadINode

	; read '/' into ex:bx (all dir_entrys, each file is mapped to a dir_entry)
	mov	dword [disk_addr_packet +  8], eax
	call	ReadHDSector

	; ds:si -> loader_name == "hdloader.bin"
	mov	si, loader_name

	push	bx		; <- save

; let's search `/' for the hdloader.bin
compare_file_name:
	; es:bx -> start position of dir_entry at root directry
	add	bx, [fs:SB_DIR_ENT_FNAME_OFF]

compare_next_byte:
	lodsb				; load a byte from ds:si -> al, also si + 1
	cmp	al, byte [es:bx]
	jz	compare_null_byte
	jmp	compare_next_file

compare_null_byte:					; so far so good
	cmp	al, 0			; both string end with '\0' and arrive at a '\0', match
	jz	loader_found
	inc	bx
	jmp	compare_next_byte

compare_next_file:
	pop	bx		; -> restore
	add	bx, [fs:SB_DIR_ENT_SIZE]
	sub	ecx, [fs:SB_DIR_ENT_SIZE]
	jz	no_loader_found

	mov	dx, SECT_BUF_SIZE
	cmp	bx, dx
	jge	no_loader_found

	push	bx
	mov	si, loader_name
	jmp	compare_file_name

no_loader_found:
	mov	dh, 2
	call	PrintStr
	jmp	$

loader_found:
	pop	bx
	add	bx, [fs:SB_DIR_ENT_INODE_OFF]
	mov	eax, [es:bx]		; eax <- inode nr of loader
	call	ReadINode		; eax <- start sector nr of loader
	mov	dword [disk_addr_packet +  8], eax
	jmp load_loader

load_loader:
	call	ReadHDSector
	cmp	ecx, SECT_BUF_SIZE
	jl	loader_loaded
	sub	ecx, SECT_BUF_SIZE	; bytes_left -= SECT_BUF_SIZE
	add	word  [disk_addr_packet + 4], SECT_BUF_SIZE ; transfer buffer
	jc	loader_load_error
	add	dword [disk_addr_packet + 8], TRANSFER_SECT_NR ; LBA
	jmp	load_loader

loader_loaded:
	mov	dh, 1
	call	PrintStr
	jmp	LOADER_SEG:LOADER_OFFSET
	jmp	$

loader_load_error:
	mov	dh, 3				; "Error 0  "
	call	PrintStr		; display the string
	jmp	$

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

;----------------------------------------------------------------------------
; ReadHDSector
;----------------------------------------------------------------------------
; Entry:
;     - fields disk_addr_packet should have been filled before invoking the routine
; Exit:
;     - es:bx -> data read
; registers changed:
;     - eax, ebx, dl, si, es
; ReadHDSector: Reads a sector from the hard disk
ReadHDSector:
    xor ebx, ebx                	; Clear ebx register (used for error code)

    mov ah, 0x42                	; AH = 0x42, the BIOS function number for extended read
    mov dl, 0x80                	; DL = 0x80, the drive number (hard disk)

    mov si, disk_addr_packet  		; SI points to the Disk Address Packet (DAP)
    int 0x13                     	; Call BIOS interrupt 0x13 to perform the disk read

    mov ax, [disk_addr_packet + 6]  ; Move the segment address from DAP to AX
    mov es, ax                   	; Set ES (extra segment) register with the segment address
    mov bx, [disk_addr_packet + 4]  ; Move the offset address from DAP to BX

    ret                          	; Return from the function


;----------------------------------------------------------------------------
; ReadINode
; Check "inode" in include/sys/fs.h
;----------------------------------------------------------------------------
; Entry:
;     - eax    : inode nr.
; Exit:
;     - eax    : sector nr.
;     - ecx    : the_inode.inode_file_size
;     - es:ebx : inodes sector buffer
; registers changed:
;     - eax, ebx, ecx, edx
ReadINode:
	dec	eax			; eax <-  inode_nr -1
	mov	bl, [fs:SB_INODE_SIZE]
	mul	bl			; eax <- (inode_nr - 1) * INODE_SIZE
	mov	edx, SECT_BUF_SIZE
	sub	edx, dword [fs:SB_INODE_SIZE]
	cmp	eax, edx
	jg	loader_load_error
	push	eax

	mov	ebx, [fs:SB_NR_IMAP_SECTS] ; sectors
	mov	edx, [fs:SB_NR_SMAP_SECTS] ; sectors
	lea	eax, [ebx + edx + ROOT_BASE + 2] ; 2 == 1(super block) + 1(boot sector)
	mov	dword [disk_addr_packet +  8], eax
	; meanwhile, es:bx points to output data
	call	ReadHDSector

	pop	eax

	; [es:ebx+eax] -> the inode
	mov	edx, dword [fs:SB_INODE_ISIZE_OFF]
	add	edx, ebx
	add	edx, eax		; [es:edx] -> the_inode.inode_file_size
	mov	ecx, [es:edx]		; ecx <- the_inode.inode_file_size

	; es:[ebx+eax] -> the_inode.inode_sect_start
	add	ax, word [fs:SB_INODE_START_OFF]

	add	bx, ax
	mov	eax, [es:bx]
	add	eax, ROOT_BASE		; eax <- the_inode.inode_sect_start
	ret

; Strings
loader_name		db	"hdloader.bin", 0
msg_base:		db	"@HD MBR  "
msg2			db	"HD Boot  "
msg3			db	"No LOADER"
msg4			db	"Error 0  "
msg_len			equ	9
;============================================================================

times 510-($-$$) db 0; Fill the remaining space to ensure the generated binary code is exactly 512 bytes
dw 0xaa55            ; End flag
