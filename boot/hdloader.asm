org  0100h
jmp	start

%include	"bootloader.inc"
%include	"descriptor.inc"

disk_addr_packet:
	db	0x10				; [ 0] disk_addr_packet size in bytes.
	db	0					; [ 1] Reserved, must be 0.
	db	TRANSFER_SECT_NR	; [ 2] Number of blocks to transfer.
	db	0					; [ 3] Reserved, must be 0.
	dw	KERNEL_OFFSET		; [ 4] Dst Addr of transfer - Seg:Offset  (Offset)
	dw	KERNEL_SEG			; [ 6] Dst Addr of transfer - Seg:Offset  (Seg)
	dd	0					; [ 8] Starting LBA address. Low  32-bits.
	dd	0					; [12] Starting LBA address. High 32-bits.

GDT_BASE:			Descriptor  0      , 0		, 0
GDT_FLAT_CODE:		Descriptor  0	   , 0fffffh, DESC_DB_32BITS |DESC_G_4K_LIMIT|DESC_P|DESC_S_C_D|DESC_DPL0|DESC_CR
GDT_FLAT_DATA:		Descriptor  0	   , 0fffffh, DESC_DB_32BITS |DESC_G_4K_LIMIT|DESC_P|DESC_S_C_D|DESC_DPL0|DESC_DRW
GDT_FLAT_VIDEO:		Descriptor  0B8000h, 0ffffh ,  								  DESC_P|DESC_S_C_D|DESC_DPL3|DESC_DRW

GDT_SIZE			equ	$ - GDT_BASE
GDTR				dw	GDT_SIZE - 1
					dd	LOADER_ADDR + GDT_BASE

SELECTOR_FLAT_CODE 	equ (0x0001<<3) + TI_GDT + RPL0
SELECTOR_FLAT_DATA 	equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO		equ (0x0003<<3) + TI_GDT + RPL3

start:
	mov	sp, STACK_BOTTOM_LOADER
	call InitSegRegs

	mov		dh, 0
	call	PrintStr

	call 	GetMemInfo

	; get Inode nr of root directory (root directory is also a file)
	mov	eax, [fs:SB_ROOT_INODE_NR]
	; get the sector nr of '/' (ROOT_INODE_NR),
	; it'll be stored in eax
	call	ReadINode

	; read '/' into ex:bx (all dir_entrys, each file is mapped to a dir_entry)
	mov	dword [disk_addr_packet +  8], eax
	call	ReadHDSector

	; ds:si -> kernel_name == "kernel.bin"
	mov	si, kernel_name

	push	bx		; <- save

; let's search `/' for the kernel.bin
compare_file_name:
	; es:bx -> start position of dir_entry at root directry
	add	bx, [fs:SB_DIR_ENT_FNAME_OFF]

compare_next_byte:
	lodsb				; load a byte from ds:si -> al, also si + 1
	cmp	al, byte [es:bx]
	jz	compare_null_byte
	jmp	compare_next_file

compare_null_byte:
	cmp	al, 0			; both string end with '\0' and arrive at a '\0', match
	jz	kernel_found
	inc	bx
	jmp	compare_next_byte

compare_next_file:
	pop	bx		; -> restore
	add	bx, [fs:SB_DIR_ENT_SIZE]
	sub	ecx, [fs:SB_DIR_ENT_SIZE]
	jz	no_kernel_found

	mov	dx, SECT_BUF_SIZE
	cmp	bx, dx
	jge	no_kernel_found

	push	bx
	mov	si, kernel_name
	jmp	compare_file_name

no_kernel_found:
	mov	dh, 2
	call	PrintStr
	jmp	$

kernel_found:
	pop	bx
	add	bx, [fs:SB_DIR_ENT_INODE_OFF]
	mov	eax, [es:bx]		; eax <- inode nr of kernel
	call	ReadINode		; eax <- start sector nr of kernel
	mov	dword [disk_addr_packet +  8], eax
	jmp load_kernel

load_kernel:
	call	ReadHDSector
	cmp	ecx, SECT_BUF_SIZE
	jl	.kernel_loaded
	sub	ecx, SECT_BUF_SIZE	; bytes_left -= SECT_BUF_SIZE
	add	word  [disk_addr_packet + 4], SECT_BUF_SIZE ; transfer buffer
	jc	.1
	jmp	.2
.1:
	add	word  [disk_addr_packet + 6], 1000h
.2:
	add	dword [disk_addr_packet + 8], TRANSFER_SECT_NR ; LBA
	jmp	load_kernel

.kernel_loaded:
	mov	dh, 2
	call	PrintStr
	jmp enter_protected_mode

kernel_load_error:
	mov	dh, 5			; "Error 0  "
	call	PrintStr	; display the string
	jmp	$

InitSegRegs:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret

GetMemInfo:
    mov ebx, 0              ; ebx = subsequent value, initialized to 0 at the beginning
    mov di, _MemChkBuf      ; es:di points to an Address Range Descriptor Structure (ARDS)

.mem_read_loop:
    mov eax, 0E820h         ; eax = 0000E820h
    mov ecx, 20             ; ecx = size of the Address Range Descriptor Structure
    mov edx, 0534D4150h     ; edx = 'SMAP'
    int 15h                 ; int 15h
    jc .mem_read_fail
    add di, 20
    inc dword [_dwMCRNumber] ; _dwMCRNumber = Number of ARDS structures
    cmp ebx, 0
    jne .mem_read_loop
    jmp .mem_read_pass

.mem_read_fail:
    mov dword [_dwMCRNumber], 0

.mem_read_pass:
    ret

;Step 2, enter protected mode
enter_protected_mode:
    lgdt [GDTR]                   ; Load the Global Descriptor Table (GDT) with lgdt instruction
    call EnableA20               ; Call the function to enable the A20 address line

    mov eax, cr0                  ; Move the value of the control register CR0 into eax
    or eax, 1                     ; Set the least significant bit to 1 to enable protected mode
    mov cr0, eax                  ; Move the modified value back to the control register CR0

    jmp dword SELECTOR_FLAT_CODE:(LOADER_ADDR + init_kernel)
                                 ; Jump to the start of the protected mode code

; Activate the A20 address line
EnableA20:
    cli                  ; Disable interrupts to ensure atomic operation

    in al, 0x92          ; Read the keyboard controller status register
    or al, 2             ; Set the A20 address line enable flag
    out 0x92, al         ; Write the modified value back to the keyboard controller

    ;sti                  ; Enable interrupts again
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
	mov	ax, msg_len
	mul	dh
	add	ax, msg_base
	mov	bp, ax
	mov	ax, ds
	mov	es, ax
	mov	cx, msg_len
	mov	ax, 0x1301		; AH = 0x13,  AL = 0x1
	mov	bx, 0x7
	mov	dl, 0
	add	dh, 3
	int	0x10
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
	mov	dword [disk_addr_packet + 12], 0

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
;----------------------------------------------------------------------------
; before:
;     - eax    : inode nr.
; after:
;     - eax    : sector nr.
;     - ecx    : the_inode.inode_file_size
;     - es:ebx : inodes sector buffer
; registers changed:
;     - eax, ebx, ecx, edx
ReadINode:
	dec	eax				; eax <-  inode_nr -1
	mov	bl, [fs:SB_INODE_SIZE]
	mul	bl				; eax <- (inode_nr - 1) * INODE_SIZE
	mov	edx, SECT_BUF_SIZE
	sub	edx, dword [fs:SB_INODE_SIZE]
	cmp	eax, edx
	jg	kernel_load_error
	push	eax

	mov	ebx, [fs:SB_NR_IMAP_SECTS]
	mov	edx, [fs:SB_NR_SMAP_SECTS]
	lea	eax, [ebx+edx+ROOT_BASE+2]
	mov	dword [disk_addr_packet +  8], eax
	call	ReadHDSector

	pop	eax				; [es:ebx+eax] -> the inode

	mov	edx, dword [fs:SB_INODE_ISIZE_OFF]
	add	edx, ebx
	add	edx, eax			; [es:edx] -> the_inode.inode_file_size
	mov	ecx, [es:edx]			; ecx <- the_inode.inode_file_size

	add	ax, word [fs:SB_INODE_START_OFF]; es:[ebx+eax] -> the_inode.inode_sect_start

	add	bx, ax
	mov	eax, [es:bx]
	add	eax, ROOT_BASE			; eax <- the_inode.inode_sect_start
	ret

; Variables
wSectorNo			dw	0
bOdd				db	0
dwKernelSize		dd	0

; Strings
kernel_name		db	"kernel.bin", 0
msg_len	equ	10
msg_base:	db	"@HD LOADER"
msg_1		db	"          "
msg_2		db	"LOADING..."
msg_3		db	"No KERNEL "
msg_4		db	"Too Large "
msg_5		db	"Error 0   "

;Step 3, run kernel
[SECTION .s32]
ALIGN	32
[BITS	32]

init_kernel:
	mov	esp, TopOfStack
	call	InitSegRegs32
	call	PrintRAMInfo
	call	SetupPaging
	call	ParseKernelELF
	call	InitBootParameter
	call	JumpToKernel

PrintAL2Hex:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.begin:
	and	al, 01111b
	cmp	al, 9
	ja	.1
	add	al, '0'
	jmp	.2
.1:
	sub	al, 0Ah
	add	al, 'A'
.2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.begin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret

PrintInt2Hex:
	mov	eax, [esp + 4]
	shr	eax, 24
	call	PrintAL2Hex

	mov	eax, [esp + 4]
	shr	eax, 16
	call	PrintAL2Hex

	mov	eax, [esp + 4]
	shr	eax, 8
	call	PrintAL2Hex

	mov	eax, [esp + 4]
	call	PrintAL2Hex

	mov	ah, 07h
	mov	al, 'h'
	push	edi
	mov	edi, [dwDispPos]
	mov	[gs:edi], ax
	add	edi, 4
	mov	[dwDispPos], edi
	pop	edi

	ret

PrintStrViaMMIO:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi

	mov	esi, [ebp + 8]
	mov	edi, [dwDispPos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[dwDispPos], edi

	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

PrintNewLine:
	push	szReturn
	call	PrintStrViaMMIO
	add	esp, 4

	ret

MemCpy:
    push    ebp            	 ; Save the caller's base pointer
    mov     ebp, esp       	 ; Set the base pointer for the current function

    push    esi            	 ; Save the value of esi register
    push    edi            	 ; Save the value of edi register
    push    ecx            	 ; Save the value of ecx register

    mov     edi, [ebp + 8]   ; Get the destination address
    mov     esi, [ebp + 12]  ; Get the source address
    mov     ecx, [ebp + 16]  ; Get the byte count

    test    ecx, ecx         ; Check if the counter is zero
    jz      end_loop         ; If zero, jump to the label end_loop to exit the loop

loop_start:
    mov     al, [esi]        ; Move one byte from the source to register al
    inc     esi              ; Increment the source address to point to the next byte
    mov     [edi], al        ; Move the byte from register al to the destination address
    inc     edi              ; Increment the destination address to point to the next byte

    dec     ecx              ; Decrement the counter
    jnz     loop_start       ; If the counter is not zero, jump back to loop_start to continue the loop

end_loop:
    mov     eax, [ebp + 8]   ; Set the destination address as the return value

    pop     ecx              ; Restore the saved value of the ecx register
    pop     edi              ; Restore the saved value of the edi register
    pop     esi              ; Restore the saved value of the esi register
    mov     esp, ebp         ; Restore the stack pointer
    pop     ebp              ; Restore the saved base pointer

    ret                      ; Return

InitSegRegs32:
	mov	ax, SELECTOR_VIDEO
	mov	gs, ax
	mov	ax, SELECTOR_FLAT_DATA
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	ss, ax
	ret

PrintRAMInfo:
    call PrintNewLine
    call PrintNewLine

    push esi
    push edi
    push ecx

    push szMemChkTitle
    call PrintStrViaMMIO
    add esp, 4

    mov esi, MemChkBuf
    mov ecx, [dwMCRNumber]   ; for(int i=0;i<[MCRNumber];i++) // Get each Address Range Descriptor Structure (ARDS) structure
.loop:                    ;{
    mov edx, 5            ;    for(int j=0;j<5;j++)    // Get each member of an ARDS, a total of 5 members
    mov edi, ARDStruct    ;    {            // Display in sequence: BaseAddrLow, BaseAddrHigh, LengthLow, LengthHigh, Type
.1:                    ;
    push dword [esi]    ;
    call PrintInt2Hex        ;        PrintInt2Hex(MemChkBuf[j*4]); // Display a member
    pop eax            ;
    stosd                ;        ARDStruct[j*4] = MemChkBuf[j*4];
    add esi, 4        ;
    dec edx            ;
    cmp edx, 0        ;
    jnz .1            ;    }
    call PrintNewLine    ;    printf("\n");
    cmp dword [dwType], 1    ;    if(Type == AddressRangeMemory) // AddressRangeMemory : 1, AddressRangeReserved : 2
    jne .2            ;    {
    mov eax, [dwBaseAddrLow]    ;
    add eax, [dwLengthLow]    ;
    cmp eax, [dwMemSize]    ;        if(BaseAddrLow + LengthLow > MemSize)
    jb .2            ;
    mov [dwMemSize], eax    ;            MemSize = BaseAddrLow + LengthLow;
.2:                    ;    }
    loop .loop            ;}
                    ;
    call PrintNewLine    ;printf("\n");
    push szRAMSize        ;
    call PrintStrViaMMIO        ;printf("RAM size:");
    add esp, 4        ;
                    ;
    push dword [dwMemSize]    ;
    call PrintInt2Hex        ;PrintInt2Hex(MemSize);
    add esp, 4        ;

    pop ecx
    pop edi
    pop esi
    ret

; Enable Paging Mechanism
SetupPaging:
	call CalculatePageTableNum
	push ecx                    ; ecx is number of page tables required
    call InitPageDirectory
    pop eax                     ; Retrieve the value of ecx from the stack
    call InitPageTables
	call EnablePaging
    ret                         ; Return from the function

; Calculate the number of page tables
CalculatePageTableNum:
    xor edx, edx                ; Clear edx to ensure correct division
    mov eax, [dwMemSize]        ; Load the memory size into eax
    mov ebx, PAGE_TABLE_SIZE    ; Load the size of a page table into ebx
    div ebx                     ; Divide memory size by page table size
    mov ecx, eax                ; Save the result in ecx (number of page tables)
    test edx, edx               ; Check if there is a remainder
    jz .no_remainder            ; If no remainder, skip incrementing ecx
    inc ecx                     ; Increment ecx to include the remainder
.no_remainder:
	ret

; Initialize the page directory
InitPageDirectory:
    mov ax, SELECTOR_FLAT_DATA  ; Load the data segment selector into ax
    mov es, ax                  ; Set es register with the data segment selector
    mov edi, PAGE_DIR_BASE      ; Load the base address of the page directory into edi
    xor eax, eax                ; Clear eax to prepare for page directory entry setup
    mov eax, PAGE_TABLE_BASE | PAGE_P | PAGE_U | PAGE_RW ; Setup page directory entry
.loop_dir:
    stosd                       ; Store the value in eax at the address in edi, and increment edi
    add eax, PAGE_SIZE          ; Move to the next page directory entry
    loop .loop_dir              ; Repeat for the specified number of entries
    ret                         ; Return from the function

; Initialize all page tables
InitPageTables:
    mov ebx, PAGE_TABLE_ENTRIES ; Load the number of entries in a page table into ebx
    mul ebx                     ; Multiply the number of page tables with the entries per table
    mov ecx, eax                ; Save the result in ecx (total number of page table entries)
    mov edi, PAGE_TABLE_BASE    ; Load the base address of the page tables into edi
    xor eax, eax                ; Clear eax to prepare for page table entry setup
    mov eax, PAGE_P | PAGE_U | PAGE_RW ; Setup page table entry
.loop_table:
    stosd                       ; Store the value in eax at the address in edi, and increment edi
    add eax, PAGE_SIZE          ; Move to the next page table entry
    loop .loop_table            ; Repeat for the specified number of entries
    ret                         ; Return from the function
; Paging mechanism is now enabled ------------------------------------------

; Enable paging mechanism
EnablePaging:
    mov eax, PAGE_DIR_BASE      ; Load the base address of the page directory into eax
    mov cr3, eax                ; Set the CR3 register with the page directory base
    mov eax, cr0                ; Load the value of CR0 register into eax
    or 	eax, PAGE_CR0_PG_BIT     ; Set the paging bit (PG) in CR0
    mov cr0, eax                ; Update CR0 register
    jmp .done                   ; Jump to the done label
.done:
    nop                         ; No operation (placeholder)
	ret

; Description: Parse KERNEL.BIN content, align and place it in a new location
; Refer to https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
ParseKernelELF:  ; Function entry point to parse ELF file
	xor	esi, esi
	mov	cx, word [KERNEL_ADDR + 2Ch]	; ┓ ecx <- pELFHdr->e_phnum
	movzx	ecx, cx					; ┛
	mov	esi, [KERNEL_ADDR + 1Ch]	; esi <- pELFHdr->e_phoff
	add	esi, KERNEL_ADDR		; esi <- OffsetOfKernel + pELFHdr->e_phoff
.Begin:
	mov	eax, [esi + 0]
	cmp	eax, 0				; PT_NULL
	jz	.NoAction
	push	dword [esi + 010h]		; size	┓
	mov	eax, [esi + 04h]		;	┃
	add	eax, KERNEL_ADDR	;	┣ ::memcpy(	(void*)(pPHdr->p_vaddr),
	push	eax				; src	┃		uchCode + pPHdr->p_offset,
	push	dword [esi + 08h]		; dst	┃		pPHdr->p_filesz;
	call	MemCpy				;	┃
	add	esp, 12				;	┛
.NoAction:
	add	esi, 020h			; esi += pELFHdr->e_phentsize
	dec	ecx
	jnz	.Begin

	ret

InitBootParameter:
	; fill in BootParam[]
	mov	dword [BOOT_PARAM_ADDR], BOOT_PARAM_MAGIC ; Magic Number
	mov	eax, [dwMemSize]
	mov	[BOOT_PARAM_ADDR + 4], eax ; memory size
	mov	eax, KERNEL_SEG
	shl	eax, 4
	add	eax, KERNEL_OFFSET
	mov	[BOOT_PARAM_ADDR + 8], eax ; phy-addr of kernel.bin
	ret

JumpToKernel:
	jmp	SELECTOR_FLAT_CODE:KRNL_ENT_PT_PHY_ADDR	;

[SECTION .data1]
ALIGN	32
data:

_szMemChkTitle:		db	"BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:			db	"RAM size: ", 0
_szReturn:			db	0Ah, 0
_dwMCRNumber:		dd	0
_dwDispPos:			dd	(80 * 7 + 0) * 2
_dwMemSize:			dd	0
_ARDStruct:			; Address Range Descriptor Structcure
	_dwBaseAddrLow:		dd	0
	_dwBaseAddrHigh:	dd	0
	_dwLengthLow:		dd	0
	_dwLengthHigh:		dd	0
	_dwType:			dd	0
_MemChkBuf:	times	256	db	0

;variables for protected mode
szMemChkTitle		equ	LOADER_ADDR + _szMemChkTitle
szRAMSize		equ	LOADER_ADDR + _szRAMSize
szReturn		equ	LOADER_ADDR + _szReturn
dwDispPos		equ	LOADER_ADDR + _dwDispPos
dwMemSize		equ	LOADER_ADDR + _dwMemSize
dwMCRNumber		equ	LOADER_ADDR + _dwMCRNumber
ARDStruct		equ	LOADER_ADDR + _ARDStruct
	dwBaseAddrLow	equ	LOADER_ADDR + _dwBaseAddrLow
	dwBaseAddrHigh	equ	LOADER_ADDR + _dwBaseAddrHigh
	dwLengthLow	equ	LOADER_ADDR + _dwLengthLow
	dwLengthHigh	equ	LOADER_ADDR + _dwLengthHigh
	dwType		equ	LOADER_ADDR + _dwType
MemChkBuf		equ	LOADER_ADDR + _MemChkBuf

StackSpace:	times	1000h	db	0
TopOfStack	equ	LOADER_ADDR + $
