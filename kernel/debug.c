#include "shared.h"

void panic(const char *fmt, ...)
{
	char buf[256];
	va_list arg = (va_list)((char*)&fmt + 4); /* 4 is the size of fmt in the stack */
	vsprintf(buf, fmt, arg);
	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);
	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void dump_proc(proc* p)
{
	char info[STR_BUFFER_LEN];
	int text_color 	= MAKE_COLOR(GREEN, RED);
	int dump_len 	= sizeof(proc);

	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, 0);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, 0);

	sprintf(info, "byte dump of proc_table[%d]:\n", p - proc_table); disp_color_str(info, text_color);
	for (int i = 0; i < dump_len; i++) {
		sprintf(info, "%x.", ((unsigned char *)p)[i]);
		disp_color_str(info, text_color);
	}

	/* printl("^^"); */

	disp_color_str("\n\n", text_color);
	sprintf(info, "ANY: 0x%x.\n", ANY); disp_color_str(info, text_color);
	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);

	sprintf(info, "ldt_sel: 0x%x.  ", p->ldt_sel); disp_color_str(info, text_color);
	sprintf(info, "ticks: 0x%x.  ", p->ticks); disp_color_str(info, text_color);
	sprintf(info, "priority: 0x%x.  ", p->priority); disp_color_str(info, text_color);
	/* sprintf(info, "pid: 0x%x.  ", p->pid); disp_color_str(info, text_color); */
	sprintf(info, "name: %s.  ", p->name); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "proc_state: 0x%x.  ", p->proc_state); disp_color_str(info, text_color);
	sprintf(info, "pid_recvingfrom: 0x%x.  ", p->pid_recvingfrom); disp_color_str(info, text_color);
	sprintf(info, "pid_sendingto: 0x%x.  ", p->pid_sendingto); disp_color_str(info, text_color);
	/* sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty); disp_color_str(info, text_color); */
	disp_color_str("\n", text_color);
	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg); disp_color_str(info, text_color);
}

void dump_msg(const char * title, message* m)
{
	int packed = 0;
	printl("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)m,
	       packed ? "" : "\n        ",
	       proc_table[m->source].name,
	       m->source,
	       packed ? " " : "\n        ",
	       m->type,
	       packed ? " " : "\n        ",
	       m->u.m3.m3i1,
	       m->u.m3.m3i2,
	       m->u.m3.m3i3,
	       m->u.m3.m3i4,
	       (int)m->u.m3.m3p1,
	       (int)m->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}