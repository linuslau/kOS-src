#ifndef _KOS_TTY_H_
#define _KOS_TTY_H_

#define TTY_FIRST			(tty_table)
#define TTY_LAST			(tty_table + NR_CONSOLES)
#define TTY_QUEUE_SIZE	256	/* tty input  queue size */

struct 	s_tty;
struct 	console;

typedef struct tty {
	u32		key_buf[TTY_QUEUE_SIZE];
	u32*	key_buf_head;
	u32*	key_buf_tail;
	int		key_buf_cnt;
	int		tty_caller;
	int		tty_procnr;
	void*	tty_req_buf;
	int		tty_left_cnt;
	int		tty_trans_cnt;
	struct 	console *	console;
}TTY;

#endif /* _KOS_TTY_H_ */
