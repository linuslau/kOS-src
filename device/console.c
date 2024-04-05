#include "shared.h"

/* #define __TTY_DEBUG__ */

static void	set_screen_cursor_position	(unsigned int position);
static void	set_screen_start_position	(u32 addr);
static void	do_screen_refresh			(CONSOLE* con);
static void	move_screen_buf				(unsigned int dst, const unsigned int src, int size);
static void	clear_screen_buf			(int pos, int len);

void init_screen(TTY* tty)
{
	int nr_tty = tty - tty_table;
	tty->console = console_table + nr_tty;

	/*variables below are in WORDs, but not in BYTEs, as every char in video buffer takes 2 bytes*/
	tty->console->w_console_start 	= W_TTY_CONSOLE_START(nr_tty);
	tty->console->w_screen_start 	= W_TTY_CONSOLE_START(nr_tty);
	tty->console->w_cursor 			= W_TTY_CONSOLE_START(nr_tty);
	tty->console->w_console_size 	= W_SIZE_PER_CONSOLE / SCR_WIDTH * SCR_WIDTH;
	tty->console->is_full 			= 0;

	if (nr_tty == 0) {
		tty->console->w_cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		const char prompt[] = "[TTY #? ](Alt+F?)\n";
		const char * p = prompt;
		for (; *p; p++)
			console_print_char(tty->console, *p == '?' ? nr_tty + '1' : *p);
	}
	set_screen_cursor_position(tty->console->w_cursor);
}

void console_print_char(CONSOLE* con, char ch)
{
	u8* cursor_video_addr 	= (u8*)(VIDEO_MEM_BASE + con->w_cursor * 2);
	assert(con->w_cursor - con->w_console_start < con->w_console_size);

	int w_cursor_x 			= (con->w_cursor - con->w_console_start) % SCR_WIDTH;
	int w_cursor_y 			= (con->w_cursor - con->w_console_start) / SCR_WIDTH;

	switch(ch) {
		case '\n':
			con->w_cursor = con->w_console_start + SCR_WIDTH * (w_cursor_y + 1);
			break;
		case '\b':
			if (con->w_cursor > con->w_console_start) {
				con->w_cursor--;
				*(cursor_video_addr - 2) = ' ';
				*(cursor_video_addr - 1) = DEFAULT_CHAR_COLOR;
			}
			break;
		default:
			*cursor_video_addr++ = ch;
			*cursor_video_addr++ = DEFAULT_CHAR_COLOR;
			con->w_cursor++;
			break;
	}

	/* console x is full */
	if (con->w_cursor - con->w_console_start >= con->w_console_size) {
		w_cursor_x = (con->w_cursor - con->w_console_start) % SCR_WIDTH;
		w_cursor_y = (con->w_cursor - con->w_console_start) / SCR_WIDTH;
		int w_screen_start = con->w_console_start + (w_cursor_y + 1) * SCR_WIDTH - SCR_SIZE;
		//place current screen to console start and set screen start to console start position.
		move_screen_buf(con->w_console_start, w_screen_start, SCR_SIZE - SCR_WIDTH);
		con->w_screen_start = con->w_console_start;
		con->w_cursor = con->w_console_start + (SCR_SIZE - SCR_WIDTH) + w_cursor_x;
		clear_screen_buf(con->w_cursor, SCR_WIDTH);
		if (!con->is_full)
			con->is_full = 1;
	}

	assert(con->w_cursor - con->w_console_start < con->w_console_size);

	while (con->w_cursor >= con->w_screen_start + SCR_SIZE || con->w_cursor < con->w_screen_start) {
		scroll_screen(con, SCROLL_DN);
		clear_screen_buf(con->w_cursor, SCR_WIDTH);
	}

	do_screen_refresh(con);
}

static void clear_screen_buf(int pos, int len)
{
	u8 * cursor_video_addr = (u8*)(VIDEO_MEM_BASE + pos * 2);
	while (--len >= 0) {
		*cursor_video_addr++ = ' ';
		*cursor_video_addr++ = DEFAULT_CHAR_COLOR;
	}
}

int is_current_console(CONSOLE* con)
{
	return (con == &console_table[current_console]);
}

static void set_screen_cursor_position(unsigned int position)
{
	disable_int	();
	out_byte	(CRTC_ADDR_REG, CURSOR_H);
	out_byte	(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte	(CRTC_ADDR_REG, CURSOR_L);
	out_byte	(CRTC_DATA_REG, position & 0xFF);
	enable_int	();
}

static void set_screen_start_position(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

void switch_console(int nr_console)
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) return;
	do_screen_refresh(&console_table[current_console = nr_console]);
}

void scroll_screen(CONSOLE* con, int dir)
{
	/* variables below are all in-console-offsets (based on con->w_console_start) */
	int w_latest_offs;
	int w_oldest_offs;
	int w_screen_start_offset;
	//it is a ring buffer
	w_latest_offs 			= (con->w_cursor - con->w_console_start) / SCR_WIDTH * SCR_WIDTH;
	w_oldest_offs 			= con->is_full ? (w_latest_offs + SCR_WIDTH) % con->w_console_size : 0;
	w_screen_start_offset 	= con->w_screen_start - con->w_console_start;

	if (dir == SCROLL_UP) {
		if (!con->is_full && w_screen_start_offset > 0) {
			con->w_screen_start -= SCR_WIDTH;
		}
		else if (con->is_full && w_screen_start_offset != w_oldest_offs) {
			if (con->w_cursor - con->w_console_start >= con->w_console_size - SCR_SIZE) {
				if (con->w_screen_start != con->w_console_start)
					con->w_screen_start -= SCR_WIDTH;
			}
			else if (con->w_screen_start == con->w_console_start) {
				w_screen_start_offset = con->w_console_size - SCR_SIZE;
				con->w_screen_start = con->w_console_start + w_screen_start_offset;
			}
			else {
				con->w_screen_start -= SCR_WIDTH;
			}
		}
	}
	else if (dir == SCROLL_DN) {
		if (!con->is_full && w_latest_offs >= w_screen_start_offset + SCR_SIZE) {
			con->w_screen_start += SCR_WIDTH;
		}
		else if (con->is_full && w_screen_start_offset + SCR_SIZE - SCR_WIDTH != w_latest_offs) {
			if (w_screen_start_offset + SCR_SIZE == con->w_console_size)
				con->w_screen_start = con->w_console_start;
			else
				con->w_screen_start += SCR_WIDTH;
		}
	}
	else {
		assert(dir == SCROLL_DN || dir == SCROLL_UP);
	}

	do_screen_refresh(con);
}

static void do_screen_refresh(CONSOLE* con)
{
	if (is_current_console(con)) {
		set_screen_cursor_position(con->w_cursor);
		set_screen_start_position(con->w_screen_start);
	}

#ifdef __TTY_DEBUG__
	int lineno = 0;
	for (lineno = 0; lineno < con->w_console_size / SCR_WIDTH; lineno++) {
		u8 * cursor_video_addr = (u8*)(VIDEO_MEM_BASE +
				   (con->w_console_start + (lineno + 1) * SCR_WIDTH) * 2
				   - 4);
		*cursor_video_addr++ = lineno / 10 + '0';
		*cursor_video_addr++ = RED_CHAR;
		*cursor_video_addr++ = lineno % 10 + '0';
		*cursor_video_addr++ = RED_CHAR;
	}
#endif
}

static	void move_screen_buf(unsigned int dst, const unsigned int src, int size /* in words */)
{
	phys_copy((void*)(VIDEO_MEM_BASE + (dst << 1)),
		  	 (void*)(VIDEO_MEM_BASE + (src << 1)),
		  	 size << 1);
}

