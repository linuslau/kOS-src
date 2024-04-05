; start.asm

extern main
extern exit

section .text

global _start

_start:
    ; Save registers that will be modified
    push eax
    push ecx

    ; Call the main function
    call main

    ; Clean up the stack
    ; Pending, this line is not required??
    add esp, 8

    ; Call the exit function
    push eax
    call exit

    ; Halt the program (should never reach here)
    hlt

; Optionally, you can include a section for data
section .data
    ; Your data declarations go here