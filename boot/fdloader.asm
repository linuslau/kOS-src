
;Step 1, find and load kernel.bin
;Step 2, enter protected mode
;Step 3, run kernel

org  0100h
jmp	start

%include	"bootloader.inc"
%include	"descriptor.inc"

SELECTOR_FLAT_CODE 	equ (0x0001<<3) + TI_GDT + RPL0
SELECTOR_FLAT_DATA 	equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO		equ (0x0003<<3) + TI_GDT + RPL3

GDT_BASE:			Descriptor        0, 0		, 0
GDT_FLAT_CODE:		Descriptor  	  0, 0fffffh, DESC_DB_32BITS |DESC_G_4K_LIMIT|DESC_P|DESC_S_C_D|DESC_DPL0|DESC_CR
GDT_FLAT_DATA:		Descriptor  	  0, 0fffffh, DESC_DB_32BITS |DESC_G_4K_LIMIT|DESC_P|DESC_S_C_D|DESC_DPL0|DESC_DRW
GDT_FLAT_VIDEO:		Descriptor  0B8000h, 0ffffh ,  								  DESC_P|DESC_S_C_D|DESC_DPL3|DESC_DRW
; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------

GDT_SIZE		equ	$ - GDT_BASE
GDTR			dw	GDT_SIZE - 1
				dd	LOADER_ADDR + GDT_BASE

;Step 1, find and load kernel.bin
start:
	mov	sp, STACK_BOTTOM_LOADER
	call 	InitSegRegs
	mov	dh, 0
	call	PrintStr
	mov	dh, 1
	call	PrintStr
	call 	GetMemInfo

    call 	ResetFloppy
    mov word [wSectorNo], 19

compare_files_in_root_dir:
	cmp	word [wRootDirSize], 0
	jz	no_kernel_found
	dec	word [wRootDirSize]

	mov	ax, KERNEL_SEG
	mov	es, ax
	mov	bx, KERNEL_OFFSET	
	mov	ax, [wSectorNo]
	mov	cl, 1
	call	ReadFDSector

	mov	si, kernel_name
	mov	di, KERNEL_OFFSET
	cld
	mov	dx, 16

compare_files_in_a_sector:
	cmp	dx, 0
	jz	compare_next_sector
	dec	dx
	mov	cx, 11

compare_file_name:
	cmp	cx, 0
	jz	kernel_found
	dec	cx
	lodsb
	cmp	al, byte [es:di]
	jz	compare_next_byte
	jmp	compare_next_file

compare_next_byte:
	inc	di
	jmp	compare_file_name

compare_next_file:
	and	di, 0FFE0h
	add	di, 20h
	mov	si, kernel_name
	jmp	compare_files_in_a_sector;   ┛

compare_next_sector:
	add	word [wSectorNo], 1
	jmp	compare_files_in_root_dir

no_kernel_found:
	mov	dh, 3
	call	PrintStr
	jmp	$

kernel_found:
	mov	ax, RootDirSectors
	and	di, 0FFF0h

	push	eax
	mov	eax, [es : di + 01Ch]
	mov	dword [dwKernelSize], eax
	cmp	eax, VALID_KERNEL_SIZE
	ja	check_kernel_size_fail
	pop	eax
	jmp	check_kernel_size_pass
check_kernel_size_fail:
	mov	dh, 4
	call	PrintStr
	jmp	$
check_kernel_size_pass:
	add	di, 01Ah
	mov	cx, word [es:di]
	push	cx
	add	cx, ax
	add	cx, DeltaSectorNo
	mov	ax, KERNEL_SEG
	mov	es, ax
	mov	bx, KERNEL_OFFSET
	mov	ax, cx

load_kernel:
	push	ax
	push	bx
	mov	ah, 0Eh
	mov	al, '.'
	mov	bl, 0Fh
	int	10h
	pop	bx
	pop	ax

	mov	cl, 1
	call	ReadFDSector
	pop	ax
	call	FAT12GetNextSector
	cmp	ax, 0FFFh
	jz	kernel_loaded
	push	ax
	mov	dx, RootDirSectors
	add	ax, dx
	add	ax, DeltaSectorNo
	add	bx, BytesPerSec
	jc	.1
	jmp	.2
.1:
	push	ax
	mov	ax, es
	add	ax, 1000h
	mov	es, ax
	pop	ax
.2:
	jmp	load_kernel

kernel_loaded:
	mov	dh, 4
	call	PrintStr
	jmp enter_protected_mode

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

;Resets the floppy drive.
ResetFloppy:
    xor ah, ah
    xor dl, dl
    int 13h
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

DrvNum              equ 0      ; Drive number for interrupt 13
RootDirSectors      equ 14     ; Space occupied by the root directory: RootDirSectors = ((BPB_RootEntCnt * 32) + (BytesPerSec – 1)) / BytesPerSec; However, if coded according to this formula, the code becomes too long
FAT1SectorNo        equ 1      ; The first sector number of FAT1 = BPB_RsvdSecCnt
DeltaSectorNo       equ 17     ; DeltaSectorNo = BPB_RsvdSecCnt + (BPB_NumFATs * FAT1Sectors) - 2
BytesPerSec         equ 512    ; Bytes per sector
SecPerTrk           equ 18     ; Sectors per track

; Variables
wRootDirSize        dw 14     ; The number of sectors occupied by the Root Directory, decremented to zero in the loop.
wSectorNo           dw 0      ; The sector number to be read.
bOdd                db 0      ; Odd or even flag
dwKernelSize        dd 0      ; Size of the KERNEL.BIN file

; Strings
kernel_name		db	"KERNEL  BIN", 0
msg_len			equ	9
msg_base		db  "@Loader  "
msg_1			db	"Loading  "
msg_2			db	"         "
msg_3			db	"         "
msg_4			db	"Ready.   "
msg_5			db	"No KERNEL"
msg_6			db	"Too Large"

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
	mov	dl, 0
    add	dh, 4
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
	mov bl, SecPerTrk   ; bl: divisor
	div bl                    ; y in al, z in ah
	inc ah                    ; z ++
	mov cl, ah                ; cl <- Starting Sector Number
	mov dh, al                ; dh <- y
	shr al, 1                 ; y >> 1 (actually y/BPB_NumHeads, where BPB_NumHeads=2)
	mov ch, al                ; ch <- Cylinder Number
	and dh, 1                 ; dh & 1 = Head Number
	pop bx                    ; Restore bx
	;Now, got Cylinder Number, Starting Sector Number, Head Number.
	mov dl, DrvNum       ; Drive Number (0 indicates A drive)
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
    mov bx, [BytesPerSec]
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

;Step 3, run kernel
[SECTION .s32]
ALIGN	32
[BITS	32]

init_kernel:
	mov		esp, TopOfStack
	call	InitSegRegs32
	call	PrintRAMInfo
	call	SetupPaging
	call	ParseKernelELF
	call 	InitBootParameter
	call 	JumpToKernel

;Functions of protected mode

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
	mov		ebp, esp
	push	ebx
	push	esi
	push	edi

	mov		esi, [ebp + 8]
	mov		edi, [dwDispPos]
	mov		ah, 0Fh
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
;variables for real mode
_szMemChkTitle:			db	"BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:				db	"RAM size: ", 0
_szReturn:				db	0Ah, 0
_dwMCRNumber:			dd	0
_dwDispPos:				dd	(80 * 7 + 0) * 2
_dwMemSize:				dd	0
_ARDStruct:				; Address Range Descriptor Structure
	_dwBaseAddrLow:		dd	0
	_dwBaseAddrHigh:	dd	0
	_dwLengthLow:		dd	0
	_dwLengthHigh:		dd	0
	_dwType:			dd	0
_MemChkBuf:	times	256	db	0

;variables for protected mode
szMemChkTitle		equ	LOADER_ADDR + _szMemChkTitle
szRAMSize			equ	LOADER_ADDR + _szRAMSize
szReturn			equ	LOADER_ADDR + _szReturn
dwDispPos			equ	LOADER_ADDR + _dwDispPos
dwMemSize			equ	LOADER_ADDR + _dwMemSize
dwMCRNumber			equ	LOADER_ADDR + _dwMCRNumber
ARDStruct			equ	LOADER_ADDR + _ARDStruct
	dwBaseAddrLow	equ	LOADER_ADDR + _dwBaseAddrLow
	dwBaseAddrHigh	equ	LOADER_ADDR + _dwBaseAddrHigh
	dwLengthLow		equ	LOADER_ADDR + _dwLengthLow
	dwLengthHigh	equ	LOADER_ADDR + _dwLengthHigh
	dwType			equ	LOADER_ADDR + _dwType
MemChkBuf			equ	LOADER_ADDR + _MemChkBuf

StackSpace:	times	1000h	db	0
TopOfStack			equ	LOADER_ADDR + $

