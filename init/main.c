#include "shared.h"

/* jmp from kernel.asm::_start. */
int main()
{
	/* in kernel now */
	init_descriptors();
	init_8259A		();
	init_clock		();
	init_keyboard	();
	init_process	(); /* task tty,sys,hd,fs,pm, process init, etc */
	while(1){}
}



