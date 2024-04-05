#include "shared.h"

/* Interrupt handler */
void	divide_error();
void	single_step_exception();
void	nmi();
void	breakpoint_exception();
void	overflow();
void	bounds_check();
void	inval_opcode();
void	copr_not_available();
void	double_fault();
void	copr_seg_overrun();
void	inval_tss();
void	segment_not_present();
void	stack_exception();
void	general_protection();
void	page_fault();
void	copr_error();
void	hwint00();
void	hwint01();
void	hwint02();
void	hwint03();
void	hwint04();
void	hwint05();
void	hwint06();
void	hwint07();
void	hwint08();
void	hwint09();
void	hwint10();
void	hwint11();
void	hwint12();
void	hwint13();
void	hwint14();
void	hwint15();

void init_gdtr_idtr()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

	/* Copy the GDT from the LOADER to the new GDT */
	memcpy(&gdt,				   			/* New GDT */
			(void*)(*((u32*)(&gdtr[2]))),   /* Base  of Old GDT */
			*((u16*)(&gdtr[0])) + 1	   		/* Limit of Old GDT */
			);


	/* gdtr[6] consists of 6 bytes: 0-15: Limit 16-47: Base. Used as the parameter for sgdt and lgdt. */
	u16* p_gdt_limit	= (u16*)(&gdtr[0]);
	u32* p_gdt_base  	= (u32*)(&gdtr[2]);
	*p_gdt_limit	 	= GDT_SIZE * sizeof(struct gdt_desc) - 1;
	*p_gdt_base			= (u32)&gdt;

	/* idtr[6] consists of 6 bytes: 0-15: Limit 16-47: Base. Used as the parameter for sidt and lidt. */
	u16* p_idt_limit 	= (u16*)(&idtr[0]);
	u32* p_idt_base 	= (u32*)(&idtr[2]);
	*p_idt_limit 		= IDT_SIZE * sizeof(struct gate_desc) - 1;
	*p_idt_base 	 	= (u32)&idt;
}

void init_idt_desc()
{
	/* All initialized as interrupt gates (no trap gates) */
	make_idt_desc(INT_VECTOR_DIVIDE,		DA_386IGate, divide_error,			PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_DEBUG,			DA_386IGate, single_step_exception,	PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_NMI,			DA_386IGate, nmi,					PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_BREAKPOINT,	DA_386IGate, breakpoint_exception,	PRIVILEGE_USER);
	make_idt_desc(INT_VECTOR_OVERFLOW,		DA_386IGate, overflow,				PRIVILEGE_USER);
	make_idt_desc(INT_VECTOR_BOUNDS,		DA_386IGate, bounds_check,			PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_INVAL_OP,		DA_386IGate, inval_opcode,			PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_COPROC_NOT,	DA_386IGate, copr_not_available,	PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_DOUBLE_FAULT,	DA_386IGate, double_fault,			PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_COPROC_SEG,	DA_386IGate, copr_seg_overrun,		PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_INVAL_TSS,		DA_386IGate, inval_tss,				PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_SEG_NOT,		DA_386IGate, segment_not_present,	PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_STACK_FAULT,	DA_386IGate, stack_exception,		PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_PROTECTION,	DA_386IGate, general_protection,	PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_PAGE_FAULT,	DA_386IGate, page_fault,			PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_COPROC_ERR,	DA_386IGate, copr_error,			PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 0,      DA_386IGate, hwint00,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 1,      DA_386IGate, hwint01,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 2,      DA_386IGate, hwint02,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 3,      DA_386IGate, hwint03,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 4,      DA_386IGate, hwint04,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 5,      DA_386IGate, hwint05,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 6,      DA_386IGate, hwint06,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ0 + 7,      DA_386IGate, hwint07,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 0,      DA_386IGate, hwint08,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 1,      DA_386IGate, hwint09,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 2,      DA_386IGate, hwint10,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 3,      DA_386IGate, hwint11,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 4,      DA_386IGate, hwint12,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 5,      DA_386IGate, hwint13,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 6,      DA_386IGate, hwint14,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_IRQ8 + 7,      DA_386IGate, hwint15,               PRIVILEGE_KRNL);
	make_idt_desc(INT_VECTOR_SYS_CALL,		DA_386IGate, sys_call,				PRIVILEGE_USER);
}

void init_tss_desc()
{
	/* Fill the TSS descriptors in GDT */
	memset(&tss, 0, sizeof(tss));
	tss.ss0	= SELECTOR_KERNEL_DS;
	make_gdt_desc(&gdt[INDEX_TSS],
					get_linear_addr(SELECTOR_KERNEL_DS, &tss),
					sizeof(tss) - 1,
					DA_386TSS);
	tss.iobase = sizeof(tss); /* No IO permission bitmap */
}

void init_ldt_desc()
{
	/* Fill the LDT descriptors of each proc in GDT  */
	for (int i = 0; i < NR_TASKS_NATIVE + NR_PROCS_MAX; i++) {
		memset(&proc_table[i], 0, sizeof(proc));

		proc_table[i].ldt_sel = SELECTOR_LDT_FIRST + (i << 3);
		assert(INDEX_LDT_FIRST + i < GDT_SIZE);
		make_gdt_desc(&gdt[INDEX_LDT_FIRST + i],
			  get_linear_addr(SELECTOR_KERNEL_DS, proc_table[i].ldts),
			  LDT_SIZE * sizeof(struct gdt_desc) - 1,
			  DA_LDT);
	}
}

void init_descriptors()
{
	init_gdtr_idtr();
	init_idt_desc();
	init_tss_desc();
	init_ldt_desc();
	load_gdtr_ldtr_tr();
}

void make_gdt_desc(struct gdt_desc * p_desc, u32 base, u32 limit, u16 attribute)
{
	p_desc->limit_low			= limit & 0x0FFFF;			/* Segment limit 1	(2 bytes) 		*/
	p_desc->base_low			= base & 0x0FFFF;			/* Segment base  1	(2 bytes) 		*/
	p_desc->base_mid			= (base >> 16) & 0x0FF;		/* Segment base  2	(1 byte) 		*/
	p_desc->attr1				= attribute & 0xFF;			/* Attribute 1 						*/
	p_desc->limit_high_attr2	= ((limit >> 16) & 0x0F) | 
								((attribute >> 8) & 0xF0);	/* Segment limit 2 + Attribute 2	*/
	p_desc->base_high			= (base >> 24) & 0x0FF;		/* Segment base 3	(1 byte) 		*/
}

void make_idt_desc(unsigned char vector, u8 desc_type, int_handler handler, unsigned char privilege)
{
	struct gate_desc * p_gate	= &idt[vector];
	u32 base 					= (u32)handler;
	p_gate->offset_low			= base & 0xFFFF;
	p_gate->selector			= SELECTOR_KERNEL_CS;
	p_gate->dcount				= 0;
	p_gate->attr				= desc_type | (privilege << 5);
	p_gate->offset_high			= (base >> 16) & 0xFFFF;
}

int ldt_idx_to_base_addr(proc* p, int ldt_index)
{
	struct gdt_desc * d = &p->ldts[ldt_index];
	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

u32 gdt_seg_to_base_addr(u16 segment_selector)
{
	//last 3 bits of segment selector are Ti and RPL. so >> 3.
	struct gdt_desc* p_desc = &gdt[segment_selector >> 3];
	return gdt_ldt_to_base_addr(p_desc);
}

u32 gdt_ldt_to_base_addr(struct gdt_desc* p_desc)
{
	return (p_desc->base_high << 24) | (p_desc->base_mid << 16) | (p_desc->base_low);
}

void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	int i;
	int text_color = 0x74;
	char err_description[][64] = {	"#DE Divide Error",
									"#DB RESERVED",
									"—  NMI Interrupt",
									"#BP Breakpoint",
									"#OF Overflow",
									"#BR BOUND Range Exceeded",
									"#UD Invalid Opcode (Undefined Opcode)",
									"#NM Device Not Available (No Math Coprocessor)",
									"#DF Double Fault",
									"    Coprocessor Segment Overrun (reserved)",
									"#TS Invalid TSS",
									"#NP Segment Not Present",
									"#SS Stack-Segment Fault",
									"#GP General Protection",
									"#PF Page Fault",
									"—  (Intel reserved. Do not use.)",
									"#MF x87 FPU Floating-Point Error (Math Fault)",
									"#AC Alignment Check",
									"#MC Machine Check",
									"#XF SIMD Floating-Point Exception"
								};

	disp_pos = 0;
	for(i=0;i<80*5;i++){
		disp_str(" ");
	}
	disp_pos = 0;

	disp_color_str("Exception! --> ", text_color);
	disp_color_str(err_description[vec_no], text_color);
	disp_color_str("\n\n", text_color);
	disp_color_str("EFLAGS:", text_color);
	disp_int(eflags);
	disp_color_str("CS:", text_color);
	disp_int(cs);
	disp_color_str("EIP:", text_color);
	disp_int(eip);

	if(err_code != 0xFFFFFFFF){
		disp_color_str("Error code:", text_color);
		disp_int(err_code);
	}
}

