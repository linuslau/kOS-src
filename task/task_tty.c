#include "shared.h"
#include "keyboard.h"

static void	tty_init			(TTY* tty);
static void	tty_dev_read		(TTY* tty);
static void	tty_dev_write		(TTY* tty);
static void tty_key_queue_init	(TTY* tty);
static void	tty_do_read			(TTY* tty, message* msg);
static void	tty_do_write		(TTY* tty, message* msg);
static void	tty_key_enqueue		(TTY* tty, u32 key);

/* Ring 1 */
void task_tty()
{
	TTY *	tty;
	message msg;

	init_keyboard();

	for (tty = TTY_FIRST; tty < TTY_LAST; tty++)
		tty_init(tty);

	/* console 1 is for basic infomation printing, not interactive */
	switch_console(CONSOLE_2);

	while (1) {
		for (tty = TTY_FIRST; tty < TTY_LAST; tty++) {
			do {
				tty_dev_read(tty);
				tty_dev_write(tty);
			} while (tty->key_buf_cnt);
		}

		ipc_msg_send_rcv(RECEIVE, ANY, &msg);

		int pid = msg.source;
		assert(pid != TASK_TTY);

		TTY* ptty = &tty_table[msg.NR_DEV_MINOR];

		switch (msg.type) {
			case DEV_OPEN:
				reset_msg(&msg);
				msg.type = SYSCALL_RET;
				ipc_msg_send_rcv(SEND, pid, &msg);
				break;
			case DEV_READ:
				tty_do_read(ptty, &msg);
				break;
			case DEV_WRITE:
				tty_do_write(ptty, &msg);
				break;
			case HW_INT:
				key_pressed = 0;
				continue;
			default:
				dump_msg("TTY::unknown msg", &msg);
				break;
		}
	}
}

static void tty_key_queue_init(TTY* tty)
{
	tty->key_buf_cnt = 0;
	tty->key_buf_head = tty->key_buf_tail = tty->key_buf;
}

static void tty_init(TTY* tty)
{
	tty_key_queue_init(tty);
	init_screen(tty);
}

void tty_process_key(TTY* tty, u32 key)
{
	if (!(key & FLAG_EXT)) {
		tty_key_enqueue(tty, key);
	}
	else {
		int raw_code = key & MASK_RAW;
		switch(raw_code) {
		case ENTER:
			tty_key_enqueue(tty, '\n');
			break;
		case BACKSPACE:
			tty_key_enqueue(tty, '\b');
			break;
		case UP:
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {	
				scroll_screen(tty->console, SCROLL_UP); /* Shift + Up */
			}
			break;
		case DOWN:
			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
				scroll_screen(tty->console, SCROLL_DN);	/* Shift + Down */
			}
			break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
				switch_console(raw_code - F1); /* Alt + F1~F12 */
			}
			break;
		default:
			break;
		}
	}
}

static void tty_key_enqueue(TTY* tty, u32 key)
{
	if (tty->key_buf_cnt < TTY_QUEUE_SIZE) {
		*(tty->key_buf_head) = key;
		tty->key_buf_head++;
		if (tty->key_buf_head == tty->key_buf + TTY_QUEUE_SIZE)
			tty->key_buf_head = tty->key_buf;
		tty->key_buf_cnt++;
	}
}

static void tty_dev_read(TTY* tty)
{
	if (is_current_console(tty->console))
		keyboard_read(tty);
}

static void tty_dev_write(TTY* tty)
{
	while (tty->key_buf_cnt) {
		char ch = *(tty->key_buf_tail);
		tty->key_buf_tail++;
		if (tty->key_buf_tail == tty->key_buf + TTY_QUEUE_SIZE)
			tty->key_buf_tail = tty->key_buf;
		tty->key_buf_cnt--;

		if (tty->tty_left_cnt) {
			if (ch >= ' ' && ch <= '~') { /* printable */
				console_print_char(tty->console, ch);
				void * p = tty->tty_req_buf +
					   tty->tty_trans_cnt;
				phys_copy(p, (void *)proc_va2la(TASK_TTY, &ch), 1);
				tty->tty_trans_cnt++;
				tty->tty_left_cnt--;
			}
			else if (ch == '\b' && tty->tty_trans_cnt) {
				console_print_char(tty->console, ch);
				tty->tty_trans_cnt--;
				tty->tty_left_cnt++;
			}

			if (ch == '\n' || tty->tty_left_cnt == 0) {
				console_print_char(tty->console, '\n');
				message msg;
				msg.type = RESUME_PROC;
				msg.PROC_NR = tty->tty_procnr;
				msg.CNT = tty->tty_trans_cnt;
				ipc_msg_send_rcv(SEND, tty->tty_caller, &msg);
				tty->tty_left_cnt = 0;
			}
		}
	}
}

static void tty_do_read(TTY* tty, message* msg)
{
	/* tell the tty: */
	tty->tty_caller   	= msg->source;  	/* who called, usually FS */
	tty->tty_procnr   	= msg->PROC_NR; 	/* who wants the chars */
	tty->tty_req_buf  	= proc_va2la(tty->tty_procnr, msg->BUF); /* where the chars should be put */
	tty->tty_left_cnt 	= msg->CNT; 		/* how many chars are requested */
	tty->tty_trans_cnt	= 0;			 	/* how many chars have been transferred */

	msg->type 			= SUSPEND_PROC;
	msg->CNT  			= tty->tty_left_cnt;
	ipc_msg_send_rcv(SEND, tty->tty_caller, msg);
}

static void tty_do_write(TTY* tty, message* msg)
{
	char buf[msg->CNT];
	char * p = (char*)proc_va2la(msg->PROC_NR, msg->BUF);

	phys_copy(proc_va2la(TASK_TTY, buf), (void*)p, msg->CNT);
	for (int j = 0; j < msg->CNT; j++)
		console_print_char(tty->console, buf[j]);

	msg->type = SYSCALL_RET;
	ipc_msg_send_rcv(SEND, msg->source, msg);
}

int sys_printx(int _unused1, int _unused2, char* s, proc* p_proc)
{
	const char * p;
	char ch;

	if (nested_int == 0)  		/* sys_printx() called in Ring 1~3 */
		p = proc_va2la(get_proc_pid(p_proc), s);
	else if (nested_int > 0)	/* sys_printx() called in Ring 0 */
		p = s;

	while ((ch = *p++) != 0) {
		console_print_char(TTY_FIRST->console, ch);
	}
	return 0;
}

void wake_up_tty()
{
	hw_int_notification(TASK_TTY);
}