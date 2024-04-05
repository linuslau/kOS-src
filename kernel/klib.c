#include "shared.h"
#include "config.h"
#include "elf.h"

int get_ticks()
{
	message msg;
	reset_msg(&msg);
	msg.type = SYS_GET_TICKS;
	ipc_msg_send_rcv(SEND_RECEIVE, TASK_SYS, &msg);
	return msg.RETVAL;
}

/* Ring 0~1 */
void get_boot_params(struct boot_params * bp)
{
	/* params saved in include/fdloader.inc boot/loader.asm boot/hdloader.asm */
	int * p_bp					= (int*)BOOT_PARAM_ADDR;
	assert(p_bp[BP_IDX_MAGIC] 	== BOOT_PARAM_MAGIC);

	bp->total_ram_size			= p_bp[BP_IDX_RAM_SIZE];
	bp->kernel_start_addr 		= (unsigned char *)(p_bp[BP_IDX_KERNEL_ADDR]);

	assert(memcmp(bp->kernel_start_addr, ELFMAG, SELFMAG) == 0);
}

/* Ring 0~1 */
int get_kernel_mem_range(unsigned int * base, unsigned int * limit)
{
	struct boot_params bp;
	get_boot_params(&bp);

	Elf32_Ehdr* elf_header 	= (Elf32_Ehdr*)(bp.kernel_start_addr);

	if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0)
		return -1;

	*base 					= ~0;
	unsigned int topmost	= 0;
	for (int i = 0; i < elf_header->e_shnum; i++) {
		Elf32_Shdr* section_header =
				(Elf32_Shdr*)(bp.kernel_start_addr + elf_header->e_shoff + i * elf_header->e_shentsize);
		if (section_header->sh_flags & SHF_ALLOC) {
			int bottom 	= section_header->sh_addr;
			int top 	= section_header->sh_addr + section_header->sh_size;

			if (*base > bottom)
				*base = bottom;
			if (topmost < top)
				topmost = top;
		}
	}
	assert(*base < topmost);
	*limit = topmost - *base - 1;

	return 0;
}

char * itoa(char * str, int num)
{
	char *	p = str;
	char	ch;
	int	i;
	int	flag = 0;

	*p++ = '0';
	*p++ = 'x';

	if(num == 0){
		*p++ = '0';
	}
	else{	
		for(i=28;i>=0;i-=4){
			ch = (num >> i) & 0xF;
			if(flag || (ch > 0)){
				flag = 1;
				ch += '0';
				if(ch > '9'){
					ch += 7;
				}
				*p++ = ch;
			}
		}
	}

	*p = 0;

	return str;
}

void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}

void delay(int time)
{
	int i, j, k;
	for(k = 0; k < time; k++){
		/*for(i=0;i<10000;i++){	for Virtual PC	*/
		for(i = 0; i < 10; i++){	/*	for Bochs	*/
			for(j = 0; j < 10000; j++){}
		}
	}
}
