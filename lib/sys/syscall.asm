; Constants
INT_VECTOR_SYS_CALL equ 0x80
NR_UNIFIED_SYS_CALL	equ 0

; Function prototype
global	sys_send_rcv

bits 32
[section .text]

; ./kernel/kernel.asm for interrupt 0x80 handler sys_call
; ./kernel/ipc.c for sys_send_rcv_handler
; ./kernel/ipc.c for kernel_msg_send / kernel_msg_rcv
sys_send_rcv:
	; Preserve registers
	push	ebx;
	push	ecx;
	push	edx;

	; Prepare syscall arguments
	mov	eax, NR_UNIFIED_SYS_CALL
	mov	ebx, [esp + 12 +  4]; function
	mov	ecx, [esp + 12 +  8]; src_dest
	mov	edx, [esp + 12 + 12]; msg

	; Invoke syscall using interrupt
	int	INT_VECTOR_SYS_CALL

	; Restore registers
	pop	edx
	pop	ecx
	pop	ebx

	ret

