#include "shared.h"

void init_clock()
{
    out_byte(TIMER_MODE, RATE_GENERATOR);
    out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
    out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));
    set_irq_handler(IRQ_TIMER, clock_handler);
    enable_irq(IRQ_TIMER);
}

/* Ring 0 */
void clock_handler(int irq)
{
    if (++sys_ticks >= MAX_TICKS)
        sys_ticks = 0;
    if (p_proc_ready->ticks)
        p_proc_ready->ticks--;
    if (key_pressed)
        wake_up_tty();
    if (nested_int != 0)
        return;
    if (p_proc_ready->ticks > 0)
        return;
    schedule();
}

/* Ring 1~3 */
void m_delay(int milli_sec)
{
    int init_ticks = get_ticks();
    while (((get_ticks() - init_ticks) * 1000 / HZ) < milli_sec) {}
}
