#include "shared.h"

static u32 get_rtc_time(struct rtc_time *t);

/* Ring 1, TASK SYS */
void task_sys()
{
    message msg;
    struct rtc_time t;

    while (1) {
        ipc_msg_send_rcv(RECEIVE, ANY, &msg);
        int src = msg.source;

        switch (msg.type) {
            case SYS_GET_TICKS:
                msg.RETVAL  = sys_ticks;
                ipc_msg_send_rcv(SEND, src, &msg);
                break;
            case SYS_GET_PID:
                msg.type    = SYSCALL_RET;
                msg.PID     = src;
                ipc_msg_send_rcv(SEND, src, &msg);
                break;
            case SYS_GET_RTC_TIME:
                msg.type    = SYSCALL_RET;
                get_rtc_time(&t);
                phys_copy(proc_va2la(src, msg.BUF),
                          proc_va2la(TASK_SYS, &t),
                          sizeof(t));
                ipc_msg_send_rcv(SEND, src, &msg);
                break;
            default:
                dump_msg("TTY::unknown msg", &msg);
                break;
        }
    }
}

static u32 get_rtc_time(struct rtc_time *t)
{
    t->year     = CMOS_READ(YEAR);
    t->month    = CMOS_READ(MONTH);
    t->day      = CMOS_READ(DAY);
    t->hour     = CMOS_READ(HOUR);
    t->minute   = CMOS_READ(MINUTE);
    t->second   = CMOS_READ(SECOND);

    if ((CMOS_READ(CLK_STATUS) & 0x04) == 0) {
        t->year     = BCD_TO_DEC(t->year);
        t->month    = BCD_TO_DEC(t->month);
        t->day      = BCD_TO_DEC(t->day);
        t->hour     = BCD_TO_DEC(t->hour);
        t->minute   = BCD_TO_DEC(t->minute);
        t->second   = BCD_TO_DEC(t->second);
    }

    t->year += 2000;

    return 0;
}
