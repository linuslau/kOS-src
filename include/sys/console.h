#ifndef _KOS_CONSOLE_H_
#define _KOS_CONSOLE_H_

/* CONSOLE */
typedef struct console {
	unsigned int	w_screen_start; 	/* set CRTC start addr reg */
	unsigned int	w_console_start;	/* start addr of the console */
	unsigned int	w_console_size;   	/* how many words does the console have */
	unsigned int	w_cursor;
	int				is_full;
} CONSOLE;

#define SCROLL_UP			1	/* scroll upward */
#define SCROLL_DN			-1	/* scroll downward */

#define SCR_SIZE			(80 * 25)
#define SCR_WIDTH			80

#define DEFAULT_CHAR_COLOR	(MAKE_COLOR(BLACK, WHITE))
#define GRAY_CHAR			(MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR			(MAKE_COLOR(BLUE, RED) | BRIGHT)

#endif /* _KOS_CONSOLE_H_ */
