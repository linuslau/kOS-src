[SECTION .text]

; Exported functions
global  memcpy
global  memset
global  strcpy
global  strlen

;void* memcpy(void* es:p_dst, void* ds:p_src, int size);
memcpy:
    push    ebp
    mov     ebp, esp

    push    esi
    push    edi
    push    ecx

    mov     edi, [ebp + 8]    	; Destination
    mov     esi, [ebp + 12]   	; Source
    mov     ecx, [ebp + 16]   	; Counter
.1:
    cmp     ecx, 0            	; Check counter
    jz      .2                	; Exit when counter is zero

    mov     al, [ds:esi]      	; ┓
    inc     esi               	; ┃
                              	; ┣ Byte-by-byte movement
    mov     byte [es:edi], al 	; ┃
    inc     edi               	; ┛

    dec     ecx               	; Decrement counter
    jmp     .1                	; Loop
.2:
    mov     eax, [ebp + 8]    	; Return value

    pop     ecx
    pop     edi
    pop     esi
    mov     esp, ebp
    pop     ebp

    ret

;void memset(void* p_dst, char ch, int size);
memset:
    push    ebp
    mov     ebp, esp

    push    esi
    push    edi
    push    ecx

    mov     edi, [ebp + 8]    	; Destination
    mov     edx, [ebp + 12]   	; Char to be putted
    mov     ecx, [ebp + 16]   	; Counter
.1:
    cmp     ecx, 0            	; Check counter
    jz      .2                	; Exit when counter is zero

    mov     byte [edi], dl    	; ┓
    inc     edi               	; ┛

    dec     ecx               	; Decrement counter
    jmp     .1                	; Loop
.2:

    pop     ecx
    pop     edi
    pop     esi
    mov     esp, ebp
    pop     ebp

    ret                       	;

;char* strcpy(char* p_dst, char* p_src);
strcpy:
    push    ebp
    mov     ebp, esp

    mov     esi, [ebp + 12]    	; Source
    mov     edi, [ebp + 8]     	; Destination

.1:
    mov     al, [esi]          	; ┓
    inc     esi                	; ┃
                               	; ┣ Byte-by-byte movement
    mov     byte [edi], al     	; ┃
    inc     edi                	; ┛

    cmp     al, 0              	; Check for '\0'
    jnz     .1                 	; Continue looping if not encountered, end if encountered

    mov     eax, [ebp + 8]     	; Return value

    pop     ebp
    ret

;int strlen(char* p_str);
strlen:
        push    ebp
        mov     ebp, esp

        mov     eax, 0			; String length starts at 0
        mov     esi, [ebp + 8]  ; esi points to the beginning address

.1:
        cmp     byte [esi], 0   ; Check if the character pointed by esi is '\0'
        jz      .2              ; If '\0' encountered, end the program
        inc     esi             ; If not '\0', point esi to the next character
        inc     eax             ; and increment eax by one
        jmp     .1              ; Loop like this

.2:
        pop     ebp
        ret