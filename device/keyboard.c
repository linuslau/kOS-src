#include "shared.h"
#include "keyboard.h"
#include "keymap.h"

// Declare an instance of KeyboardState struct
static struct KeyboardState     kb_state;
static struct KeyboardInBuffer  kb_in;

static u8   get_byte_from_kb_buf();
static void kb_wait             ();
static void kb_ack              ();
static void set_leds            ();
static void keyboard_handler    (int irq);

static void handle_pausebreak   (u32 *key);
static void handle_printscreen  (u32 *key, int *make, u8 *scan_code);
static void process_regular_key (u32 *key, int *make, u32 *keyrow, u8 scan_code);
static void process_numpad_key  (TTY* tty, u32 key, int make, u8 scan_code, u32 *keyrow);

void init_keyboard()
{
    kb_state            = (struct KeyboardState){0};
    kb_state.num_lock   = 1;
    set_leds();
    set_irq_handler(IRQ_KEYBOARD, keyboard_handler);
    enable_irq(IRQ_KEYBOARD);
}

void keyboard_handler(int irq)
{
    u8 scan_code = in_byte(KB_DATA);

    if (kb_state.count < LEN_KB_IN_BUFFER) {
        kb_state.count++;
        *(kb_in.p_head) = scan_code;
        kb_in.p_head++;
        if (kb_in.p_head == kb_in.buf + LEN_KB_IN_BUFFER)
            kb_in.p_head = kb_in.buf;
    }

    key_pressed = 1;
}

void keyboard_read(TTY* tty)
{
    u8 scan_code;
    int make;
    u32 key = 0;
    u32* keyrow = 0;

    while (kb_state.count > 0) {
        kb_state.code_with_E0 = 0;
        scan_code = get_byte_from_kb_buf();

        if (scan_code == 0xE1) {
            handle_pausebreak(&key);
        }
        else if (scan_code == 0xE0) {
            handle_printscreen(&key, &make, &scan_code);
        }

        if ((key != PAUSEBREAK) && (key != PRINTSCREEN)) {
            process_regular_key(&key, &make, keyrow, scan_code);
        }

        if (make) {
            process_numpad_key(tty, key, make, scan_code, keyrow);
        }
    }
}

static void handle_pausebreak(u32 *key)
{
    u8 pausebreak_scan_code[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
    int is_pausebreak = 1;

    for (int i = 1; i < sizeof(pausebreak_scan_code); i++) {
        if (get_byte_from_kb_buf() != pausebreak_scan_code[i]) {
            is_pausebreak = 0;
            break;
        }
    }

    if (is_pausebreak) {
        *key = PAUSEBREAK;
    }
}

static void handle_printscreen(u32 *key, int *make, u8 *scan_code)
{
    kb_state.code_with_E0 = 1;
    *scan_code = get_byte_from_kb_buf();

    if (*scan_code == 0x2A) { /* pressed */
        kb_state.code_with_E0 = 0;
        if ((*scan_code = get_byte_from_kb_buf()) == 0xE0) {
            kb_state.code_with_E0 = 1;
            if ((*scan_code = get_byte_from_kb_buf()) == 0x37) {
                *key = PRINTSCREEN;
                *make = 1;
            }
        }
    }
    else if (*scan_code == 0xB7) { /* released */
        kb_state.code_with_E0 = 0;
        if ((*scan_code = get_byte_from_kb_buf()) == 0xE0) {
            kb_state.code_with_E0 = 1;
            if ((*scan_code = get_byte_from_kb_buf()) == 0xAA) {
                *key = PRINTSCREEN;
                *make = 0;
            }
        }
    }
}

static void process_regular_key(u32 *key, int *make, u32 *keyrow, u8 scan_code)
{
    if ((*key != PAUSEBREAK) && (*key != PRINTSCREEN)) {
        int caps;   
        *make = (scan_code & FLAG_BREAK ? 0 : 1);
        keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];
        kb_state.column = 0;
        caps = kb_state.shift_l || kb_state.shift_r;

        if (kb_state.caps_lock && keyrow[0] >= 'a' && keyrow[0] <= 'z') {
            caps = !caps;
        }

        if (caps) {
            kb_state.column = 1;
        }

        if (kb_state.code_with_E0) {
            kb_state.column = 2;
        }

        *key = keyrow[kb_state.column];

        switch (*key) {
            case SHIFT_L:
                kb_state.shift_l = *make;
                break;
            case SHIFT_R:
                kb_state.shift_r = *make;
                break;
            case CTRL_L:
                kb_state.ctrl_l = *make;
                break;
            case CTRL_R:
                kb_state.ctrl_r = *make;
                break;
            case ALT_L:
                kb_state.alt_l = *make;
                break;
            case ALT_R:
                kb_state.alt_l = *make;
                break;
            case CAPS_LOCK:
                if (*make) {
                    kb_state.caps_lock = !kb_state.caps_lock;
                    set_leds();
                }
                break;
            case NUM_LOCK:
                if (*make) {
                    kb_state.num_lock = !kb_state.num_lock;
                    set_leds();
                }
                break;
            case SCROLL_LOCK:
                if (*make) {
                    kb_state.scroll_lock = !kb_state.scroll_lock;
                    set_leds();
                }
                break;
            default:
                break;
        }
    }
}

static void process_numpad_key(TTY* tty, u32 key, int make, u8 scan_code, u32 *keyrow)
{
    int pad = 0;

    if ((key >= PAD_SLASH) && (key <= PAD_9)) {
        pad = 1;

        switch (key) {
            case PAD_SLASH:
                key = '/';
                break;
            case PAD_STAR:
                key = '*';
                break;
            case PAD_MINUS:
                key = '-';
                break;
            case PAD_PLUS:
                key = '+';
                break;
            case PAD_ENTER:
                key = ENTER;
                break;
            default:
                if (kb_state.num_lock) {
                    if (key >= PAD_0 && key <= PAD_9) {
                        key = key - PAD_0 + '0';
                    }
                    else if (key == PAD_DOT) {
                        key = '.';
                    }
                }
                else {
                    switch (key) {
                        case PAD_HOME:
                            key = HOME;
                            break;
                        case PAD_END:
                            key = END;
                            break;
                        case PAD_PAGEUP:
                            key = PAGEUP;
                            break;
                        case PAD_PAGEDOWN:
                            key = PAGEDOWN;
                            break;
                        case PAD_INS:
                            key = INSERT;
                            break;
                        case PAD_UP:
                            key = UP;
                            break;
                        case PAD_DOWN:
                            key = DOWN;
                            break;
                        case PAD_LEFT:
                            key = LEFT;
                            break;
                        case PAD_RIGHT:
                            key = RIGHT;
                            break;
                        case PAD_DOT:
                            key = DELETE;
                            break;
                        default:
                            break;
                    }
                }
                break;
        }
    }

    key |= kb_state.shift_l ? FLAG_SHIFT_L : 0;
    key |= kb_state.shift_r ? FLAG_SHIFT_R : 0;
    key |= kb_state.ctrl_l ? FLAG_CTRL_L : 0;
    key |= kb_state.ctrl_r ? FLAG_CTRL_R : 0;
    key |= kb_state.alt_l ? FLAG_ALT_L : 0;
    key |= kb_state.alt_r ? FLAG_ALT_R : 0;
    key |= pad ? FLAG_PAD : 0;

    tty_process_key(tty, key);
}

static u8 get_byte_from_kb_buf()
{
    u8 scan_code;

    while (kb_state.count <= 0) {} /* wait for a byte to arrive */

    disable_int();  /* for synchronization */
    scan_code = *(kb_in.p_tail);
    kb_in.p_tail++;
    if (kb_in.p_tail == kb_in.buf + LEN_KB_IN_BUFFER) {
        kb_in.p_tail = kb_in.buf;
    }
    kb_state.count--;
    enable_int();   /* for synchronization */

    return scan_code;
}

static void kb_wait()  /* Wait for the 8042 input buffer to be empty */
{
    u8 kb_read;

    do {
        kb_read = in_byte(KB_CMD);
    } while (kb_read & KB_IN_BUFFER_EMPTY);
}

static void kb_ack()
{
    u8 kb_read;

    do {
        kb_read = in_byte(KB_DATA);
    } while (kb_read != KB_ACK);
}

static void set_leds()
{
    u8 leds = (kb_state.caps_lock << 2) | (kb_state.num_lock << 1) | kb_state.scroll_lock;

    kb_wait();
    out_byte(KB_DATA, LED_CODE);
    kb_ack();

    kb_wait();
    out_byte(KB_DATA, leds);
    kb_ack();
}