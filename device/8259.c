#include "shared.h"

void init_8259A()
{
	out_byte(INT_M_CTL, 0x11);		 			/* Master 8259, ICW1. */
	out_byte(INT_S_CTL, 0x11); 					/* Slave 8259, ICW1. */
	out_byte(INT_M_CTLMASK, INT_VECTOR_IRQ0); 	/* Master 8259, ICW2. Set the interrupt entry address of 'Master 8259' to 0x20. */
	out_byte(INT_S_CTLMASK, INT_VECTOR_IRQ8); 	/* Slave 8259, ICW2. Set the interrupt entry address of 'Slave 8259' to 0x28. */
	out_byte(INT_M_CTLMASK, 0x4); 				/* Master 8259, ICW3. IR2 corresponds to 'Slave 8259'. */
	out_byte(INT_S_CTLMASK, 0x2); 				/* Slave 8259, ICW3. Corresponds to IR2 of 'Master 8259'. */
	out_byte(INT_M_CTLMASK, 0x1); 				/* Master 8259, ICW4. */
	out_byte(INT_S_CTLMASK, 0x1); 				/* Slave 8259, ICW4. */
	out_byte(INT_M_CTLMASK, 0xFF); 				/* Master 8259, OCW1. */
	out_byte(INT_S_CTLMASK, 0xFF); 				/* Slave 8259, OCW1. */
	set_default_irq_handler();
}

void set_default_irq_handler()
{
	for (int i = 0; i < NR_IRQ; i++) {
		set_irq_handler(i, default_irq_handler);
	}
}

void set_irq_handler(int irq, irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}

void default_irq_handler(int irq)
{
	disp_str("default_irq_handler: ");
	disp_int(irq);
	disp_str("\n");
}
