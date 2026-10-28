/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pw_osk.h"
#include <string.h>

PwOsk g_pw_osk;

size_t pw_osk_tiled(unsigned x, unsigned y)
{
    const unsigned offset = ((y << 4) & 0x70) ^ ((y << 5) & 0xf00) ^ ((y << 9) & 0x1000) ^ ((y << 8) & 0x4000) ^
                            ((x << 2) & 0xc) ^ ((x << 5) & 0x380) ^ ((x << 4) & 0x400) ^ ((x << 6) & 0x800) ^
                            ((x << 9) & 0xa000);
    return ((size_t)((y >> 7) * PW_OSK_DISPLAY_BLOCKS + (x >> 7)) << 16) + offset;
}

/* 5x7 bitmap font for ASCII 32 (' ') to 95 ('_') */
static const uint8_t kFont5x7[][7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 32 ' ' */
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}, /* 33 '!' */
    {0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00}, /* 34 '"' */
    {0x0a, 0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x0a}, /* 35 '#' */
    {0x04, 0x0f, 0x14, 0x0e, 0x05, 0x1e, 0x04}, /* 36 '$' */
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}, /* 37 '%' */
    {0x0c, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0d}, /* 38 '&' */
    {0x0c, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}, /* 39 '\'' */
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}, /* 40 '(' */
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}, /* 41 ')' */
    {0x00, 0x04, 0x15, 0x0e, 0x15, 0x04, 0x00}, /* 42 '*' */
    {0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00}, /* 43 '+' */
    {0x00, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x08}, /* 44 ',' */
    {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}, /* 45 '-' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}, /* 46 '.' */
    {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00}, /* 47 '/' */
    {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}, /* 48 '0' */
    {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}, /* 49 '1' */
    {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}, /* 50 '2' */
    {0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e}, /* 51 '3' */
    {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}, /* 52 '4' */
    {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e}, /* 53 '5' */
    {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e}, /* 54 '6' */
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, /* 55 '7' */
    {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}, /* 56 '8' */
    {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}, /* 57 '9' */
    {0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00}, /* 58 ':' */
    {0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x04, 0x08}, /* 59 ';' */
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}, /* 60 '<' */
    {0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00}, /* 61 '=' */
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}, /* 62 '>' */
    {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, /* 63 '?' */
    {0x0e, 0x11, 0x01, 0x0d, 0x15, 0x15, 0x0e}, /* 64 '@' */
    {0x0e, 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11}, /* 65 'A' */
    {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}, /* 66 'B' */
    {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e}, /* 67 'C' */
    {0x1c, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1c}, /* 68 'D' */
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}, /* 69 'E' */
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}, /* 70 'F' */
    {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f}, /* 71 'G' */
    {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}, /* 72 'H' */
    {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e}, /* 73 'I' */
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c}, /* 74 'J' */
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, /* 75 'K' */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}, /* 76 'L' */
    {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}, /* 77 'M' */
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, /* 78 'N' */
    {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* 79 'O' */
    {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}, /* 80 'P' */
    {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}, /* 81 'Q' */
    {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}, /* 82 'R' */
    {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}, /* 83 'S' */
    {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* 84 'T' */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, /* 85 'U' */
    {0x11, 0x11, 0x11, 0x0a, 0x0a, 0x04, 0x04}, /* 86 'V' */
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a}, /* 87 'W' */
    {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}, /* 88 'X' */
    {0x11, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04}, /* 89 'Y' */
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}, /* 90 'Z' */
    {0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e}, /* 91 '[' */
    {0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00}, /* 92 '\' */
    {0x0e, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0e}, /* 93 ']' */
    {0x04, 0x0a, 0x11, 0x00, 0x00, 0x00, 0x00}, /* 94 '^' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f}, /* 95 '_' */
};

/* Keyboard layout definition: 5 rows */
static const PwOskKey kLayout[PW_OSK_ROWS][PW_OSK_MAX_COLS] = {
    /* Row 0: Numbers & symbols */
    {
        {'1', '!', NULL, '1', 0x02, 1}, {'2', '@', NULL, '2', 0x03, 1},
        {'3', '#', NULL, '3', 0x04, 1}, {'4', '$', NULL, '4', 0x05, 1},
        {'5', '%', NULL, '5', 0x06, 1}, {'6', '^', NULL, '6', 0x07, 1},
        {'7', '&', NULL, '7', 0x08, 1}, {'8', '*', NULL, '8', 0x09, 1},
        {'9', '(', NULL, '9', 0x0a, 1}, {'0', ')', NULL, '0', 0x0b, 1},
        {'-', '_', NULL, 0xbd, 0x0c, 1}, {'=', '+', NULL, 0xbb, 0x0d, 1}
    },
    /* Row 1: QWERTY top */
    {
        {'q', 'Q', NULL, 'Q', 0x10, 1}, {'w', 'W', NULL, 'W', 0x11, 1},
        {'e', 'E', NULL, 'E', 0x12, 1}, {'r', 'R', NULL, 'R', 0x13, 1},
        {'t', 'T', NULL, 'T', 0x14, 1}, {'y', 'Y', NULL, 'Y', 0x15, 1},
        {'u', 'U', NULL, 'U', 0x16, 1}, {'i', 'I', NULL, 'I', 0x17, 1},
        {'o', 'O', NULL, 'O', 0x18, 1}, {'p', 'P', NULL, 'P', 0x19, 1},
        {'[', '{', NULL, 0xdb, 0x1a, 1}, {']', '}', NULL, 0xdd, 0x1b, 1}
    },
    /* Row 2: ASDF middle */
    {
        {'a', 'A', NULL, 'A', 0x1e, 1}, {'s', 'S', NULL, 'S', 0x1f, 1},
        {'d', 'D', NULL, 'D', 0x20, 1}, {'f', 'F', NULL, 'F', 0x21, 1},
        {'g', 'G', NULL, 'G', 0x22, 1}, {'h', 'H', NULL, 'H', 0x23, 1},
        {'j', 'J', NULL, 'J', 0x24, 1}, {'k', 'K', NULL, 'K', 0x25, 1},
        {'l', 'L', NULL, 'L', 0x26, 1}, {';', ':', NULL, 0xba, 0x27, 1},
        {'\'', '"', NULL, 0xde, 0x28, 1}, {'\n', '\n', "Enter", 0x0d, 0x1c, 1}
    },
    /* Row 3: ZXCV bottom + Shift/Bksp */
    {
        {0, 0, "Shift", 0, 0, 1},
        {'z', 'Z', NULL, 'Z', 0x2c, 1}, {'x', 'X', NULL, 'X', 0x2d, 1},
        {'c', 'C', NULL, 'C', 0x2e, 1}, {'v', 'V', NULL, 'V', 0x2f, 1},
        {'b', 'B', NULL, 'B', 0x30, 1}, {'n', 'N', NULL, 'N', 0x31, 1},
        {'m', 'M', NULL, 'M', 0x32, 1}, {',', '<', NULL, 0xbc, 0x33, 1},
        {'.', '>', NULL, 0xbe, 0x34, 1}, {'/', '?', NULL, 0xbf, 0x35, 1},
        {0x08, 0x08, "Bksp", 0x08, 0x0e, 1}
    },
    /* Row 4: Tab, Space bar, Esc */
    {
        {0x09, 0x09, "Tab", 0x09, 0x0f, 2},
        {0x09, 0x09, "Tab", 0x09, 0x0f, 0}, /* alias for col 1 */
        {' ', ' ', "Space", 0x20, 0x39, 8},
        {' ', ' ', "Space", 0x20, 0x39, 0},
        {' ', ' ', "Space", 0x20, 0x39, 0},
        {' ', ' ', "Space", 0x20, 0x39, 0},
        {' ', ' ', "Space", 0x20, 0x39, 0},
        {' ', ' ', "Space", 0x20, 0x39, 0},
        {' ', ' ', "Space", 0x20, 0x39, 0},
        {' ', ' ', "Space", 0x20, 0x39, 0}, /* alias cols 3..9 */
        {0x1b, 0x1b, "Esc", 0x1b, 0x01, 2},
        {0x1b, 0x1b, "Esc", 0x1b, 0x01, 0}  /* alias for col 11 */
    }
};

static inline uint32_t make_key_code(uint8_t vk, uint16_t scan) {
    return (uint32_t)vk | ((uint32_t)(scan & 0xff) << 8) | ((uint32_t)(scan >> 8) << 16);
}

void pw_osk_init(PwOsk *osk)
{
    if (!osk) return;
    memset(osk, 0, sizeof(*osk));
    osk->row = 1;
    osk->col = 0;
}

int pw_osk_is_active(const PwOsk *osk)
{
    return osk ? osk->active : 0;
}

void pw_osk_open(PwOsk *osk)
{
    if (!osk) return;
    osk->active = 1;
    osk->row = 1;
    osk->col = 0;
    osk->shift = 0;
}

void pw_osk_close(PwOsk *osk)
{
    if (!osk) return;
    osk->active = 0;
}

unsigned pw_osk_update(PwOsk *osk, const struct pad *pad, unsigned keyboards_connected,
                       PwWineInput *out, unsigned max_out, uint32_t now_ms)
{
    if (!osk || !pad || !out || max_out < 4) return 0;
    unsigned count = 0;

    /* If a key was held down on previous frame, release it now */
    if (osk->pending_up_vk) {
        out[count++] = (PwWineInput){PW_WINE_INPUT_KEY, make_key_code(osk->pending_up_vk, osk->pending_up_scan), 0, 0, 0};
        if (osk->pending_up_shift) {
            out[count++] = (PwWineInput){PW_WINE_INPUT_KEY, make_key_code(0x10, 0x2a), 0, 0, 0}; /* Shift Up */
            osk->pending_up_shift = 0;
        }
        osk->pending_up_vk = 0;
        osk->pending_up_scan = 0;
    }

    /* Activation check when inactive: L3 + R3 pressed together while no physical keyboard is connected */
    uint32_t l3_r3 = (PAD_L3 | PAD_R3);
    if (!osk->active) {
        if (keyboards_connected == 0) {
            if ((pad->held & l3_r3) == l3_r3 && (osk->prev_held & l3_r3) != l3_r3) {
                osk->active = 1;
                osk->row = 1;
                osk->col = 0;
                osk->shift = 0;
                osk->last_move_ms = now_ms;
            }
        }
        osk->prev_held = pad->held;
        return count;
    }

    /* Active: check dismissal */
    uint32_t pressed = pad->held & ~osk->prev_held;
    osk->prev_held = pad->held;

    /* User rule: Press Circle to remove / close the keyboard */
    if (pressed & PAD_CIRCLE) {
        osk->active = 0;
        return count;
    }

    /* L3 + R3 combo pressed again also closes */
    if ((pad->held & l3_r3) == l3_r3 && (osk->prev_held & l3_r3) != l3_r3) {
        osk->active = 0;
        return count;
    }

    /* Quick triggers: L2 / R2 toggles Shift */
    int trigger_pressed = (pad->l2 > 0.6f || pad->r2 > 0.6f);
    if (trigger_pressed && !osk->prev_trigger) {
        osk->shift = !osk->shift;
    }
    osk->prev_trigger = trigger_pressed;

    /* Quick buttons: Square = Backspace, Triangle = Space */
    if (pressed & PAD_SQUARE) {
        out[count++] = (PwWineInput){PW_WINE_INPUT_KEY, make_key_code(0x08, 0x0e), 0, 0, 1};
        osk->pending_up_vk = 0x08;
        osk->pending_up_scan = 0x0e;
        return count;
    }
    if (pressed & PAD_TRIANGLE) {
        out[count++] = (PwWineInput){PW_WINE_INPUT_KEY, make_key_code(0x20, 0x39), 0, 0, 1};
        osk->pending_up_vk = 0x20;
        osk->pending_up_scan = 0x39;
        return count;
    }

    /* Navigation: D-Pad & Left Stick */
    int move_x = 0, move_y = 0;
    if (pressed & PAD_LEFT) move_x = -1;
    else if (pressed & PAD_RIGHT) move_x = 1;
    else if (pad->left_x < -0.55f) move_x = -1;
    else if (pad->left_x > 0.55f) move_x = 1;

    if (pressed & PAD_UP) move_y = -1;
    else if (pressed & PAD_DOWN) move_y = 1;
    else if (pad->left_y < -0.55f) move_y = -1;
    else if (pad->left_y > 0.55f) move_y = 1;

    if (move_x != 0 || move_y != 0) {
        int is_new_press = (pressed & (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN));
        if (is_new_press || (now_ms - osk->last_move_ms >= 180)) {
            osk->last_move_ms = now_ms;
            if (move_x < 0) {
                if (osk->row == 4) {
                    if (osk->col >= 10) osk->col = 2;       /* Esc -> Space */
                    else if (osk->col >= 2) osk->col = 0;   /* Space -> Tab */
                    else osk->col = 10;                     /* Tab wrap -> Esc */
                } else {
                    osk->col = (osk->col + PW_OSK_MAX_COLS - 1) % PW_OSK_MAX_COLS;
                }
            } else if (move_x > 0) {
                if (osk->row == 4) {
                    if (osk->col < 2) osk->col = 2;         /* Tab -> Space */
                    else if (osk->col < 10) osk->col = 10;  /* Space -> Esc */
                    else osk->col = 0;                      /* Esc wrap -> Tab */
                } else {
                    osk->col = (osk->col + 1) % PW_OSK_MAX_COLS;
                }
            }
            if (move_y < 0) {
                osk->row = (osk->row + PW_OSK_ROWS - 1) % PW_OSK_ROWS;
            } else if (move_y > 0) {
                osk->row = (osk->row + 1) % PW_OSK_ROWS;
            }
        }
    }

    /* Typing: Cross presses focused key */
    if (pressed & PAD_CROSS) {
        const PwOskKey *k = &kLayout[osk->row][osk->col];
        if (k->label && !strcmp(k->label, "Shift")) {
            osk->shift = !osk->shift;
        } else if (k->vk) {
            if (osk->shift) {
                out[count++] = (PwWineInput){PW_WINE_INPUT_KEY, make_key_code(0x10, 0x2a), 0, 0, 1}; /* Shift Down */
                osk->pending_up_shift = 1;
                osk->shift = 0; /* one-shot Shift */
            }
            out[count++] = (PwWineInput){PW_WINE_INPUT_KEY, make_key_code(k->vk, k->scan), 0, 0, 1};
            osk->pending_up_vk = k->vk;
            osk->pending_up_scan = k->scan;
        }
    }

    return count;
}

/* Helper to draw pixel on linear RGBA/BGRA */
static inline void put_linear_pixel(uint32_t *pixels, unsigned w, unsigned h, unsigned pitch_pixels,
                                   unsigned x, unsigned y, uint32_t color) {
    if (x < w && y < h) {
        pixels[y * pitch_pixels + x] = color;
    }
}

/* Helper to flush cache line on x86_64 */
static inline void flush_line(const void *p) {
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile("clflushopt (%0)" : : "r"(p) : "memory");
#else
    (void)p;
#endif
}

/* Helper to draw pixel on PS5 tiled memory */
static inline void put_tiled_pixel(uint8_t *buffer, unsigned w, unsigned h,
                                  unsigned x, unsigned y, uint32_t color) {
    if (x < w && y < h) {
        uint32_t *p = (uint32_t *)(void *)(buffer + pw_osk_tiled(x, y));
        *p = color;
        flush_line(p);
    }
}

/* Renders a single 5x7 character */
static void draw_char(int tiled, void *dest, unsigned dw, unsigned dh, unsigned pitch_pixels,
                      int px, int py, char c, int scale, uint32_t color) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c < ' ' || c > '_') c = '?';
    const uint8_t *glyph = kFont5x7[c - ' '];
    for (int r = 0; r < 7; ++r) {
        uint8_t row_bits = glyph[r];
        for (int col = 0; col < 5; ++col) {
            if (row_bits & (0x10 >> col)) {
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        unsigned x = (unsigned)(px + col * scale + sx);
                        unsigned y = (unsigned)(py + r * scale + sy);
                        if (tiled) put_tiled_pixel((uint8_t *)dest, dw, dh, x, y, color);
                        else put_linear_pixel((uint32_t *)dest, dw, dh, pitch_pixels, x, y, color);
                    }
                }
            }
        }
    }
}

/* Renders a text string */
static void draw_string(int tiled, void *dest, unsigned dw, unsigned dh, unsigned pitch_pixels,
                        int px, int py, const char *str, int scale, uint32_t color) {
    if (!str) return;
    int cur_x = px;
    while (*str) {
        draw_char(tiled, dest, dw, dh, pitch_pixels, cur_x, py, *str, scale, color);
        cur_x += 6 * scale;
        str++;
    }
}

/* Draws a filled rectangle with a 1px border */
static void draw_rect(int tiled, void *dest, unsigned dw, unsigned dh, unsigned pitch_pixels,
                      int x, int y, int w, int h, uint32_t fill_color, uint32_t border_color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;

    for (int ry = 0; ry < h; ++ry) {
        for (int rx = 0; rx < w; ++rx) {
            uint32_t c = (rx == 0 || rx == w - 1 || ry == 0 || ry == h - 1) ? border_color : fill_color;
            if (tiled) put_tiled_pixel((uint8_t *)dest, dw, dh, (unsigned)(x + rx), (unsigned)(y + ry), c);
            else put_linear_pixel((uint32_t *)dest, dw, dh, pitch_pixels, (unsigned)(x + rx), (unsigned)(y + ry), c);
        }
    }
}

/* Common internal renderer for both linear and tiled displays */
static void pw_osk_render_internal(const PwOsk *osk, int tiled, void *dest,
                                   unsigned width, unsigned height, unsigned pitch_pixels)
{
    if (!osk || !osk->active || !dest) return;

    /* Overlay position: bottom center of 1920x1080 */
    int kb_x = 350, kb_y = 700, kb_w = 1220, kb_h = 340;
    uint32_t bg_fill = 0xff15181e;     /* Dark slate */
    uint32_t bg_border = 0xff3b414f;   /* Medium slate border */
    draw_rect(tiled, dest, width, height, pitch_pixels, kb_x, kb_y, kb_w, kb_h, bg_fill, bg_border);

    /* Header banner with instructions */
    const char *banner = "[D-PAD] MOVE   [X] TYPE   [CIRCLE] CLOSE   [SQUARE] BKSP   [TRIANGLE] SPACE   [L2/R2] SHIFT";
    draw_string(tiled, dest, width, height, pitch_pixels, kb_x + 30, kb_y + 14, banner, 2, 0xffa0a6b5);

    /* Render keys */
    int key_margin_x = 20, key_gap_x = 8;
    int key_w = 88, key_h = 44, key_gap_y = 6;
    int start_y = kb_y + 44;

    for (int r = 0; r < PW_OSK_ROWS; ++r) {
        int cur_x = kb_x + key_margin_x;
        int cur_y = start_y + r * (key_h + key_gap_y);

        for (int c = 0; c < PW_OSK_MAX_COLS; ++c) {
            const PwOskKey *k = &kLayout[r][c];
            if (k->span == 0) continue; /* Alias column covered by multi-column span */

            int actual_w = key_w * k->span + key_gap_x * (k->span - 1);
            int is_focused = (osk->row == r && (osk->col >= c && osk->col < c + (k->span ? k->span : 1)));

            uint32_t fill_color = is_focused ? 0xff0072ce : 0xff252830;
            uint32_t border_color = is_focused ? 0xff66b3ff : 0xff3f4552;
            uint32_t text_color = is_focused ? 0xffffffff : 0xffe2e5eb;

            if (k->label && !strcmp(k->label, "Shift") && osk->shift) {
                fill_color = 0xff2a7a40; /* Highlighted green when Shift is active */
                border_color = 0xff5cd67c;
            }

            draw_rect(tiled, dest, width, height, pitch_pixels, cur_x, cur_y, actual_w, key_h, fill_color, border_color);

            /* Key label */
            char label_buf[16] = {};
            if (k->label) {
                strncpy(label_buf, k->label, sizeof(label_buf) - 1);
            } else {
                label_buf[0] = osk->shift ? k->ch_shifted : k->ch_normal;
                label_buf[1] = '\0';
            }

            int label_len = (int)strlen(label_buf);
            int text_w = label_len * 6 * 2 - 2;
            int text_x = cur_x + (actual_w - text_w) / 2;
            int text_y = cur_y + (key_h - 14) / 2;
            draw_string(tiled, dest, width, height, pitch_pixels, text_x, text_y, label_buf, 2, text_color);

            cur_x += actual_w + key_gap_x;
        }
    }
}

void pw_osk_draw_linear(const PwOsk *osk, uint32_t *pixels, unsigned width, unsigned height, unsigned pitch)
{
    pw_osk_render_internal(osk, 0, (void *)pixels, width, height, pitch / sizeof(uint32_t));
}

void pw_osk_draw_tiled(const PwOsk *osk, uint8_t *buffer, unsigned width, unsigned height)
{
    pw_osk_render_internal(osk, 1, (void *)buffer, width, height, width);
}

void pw_osk_draw(uint8_t *target)
{
    if (!g_pw_osk.active || !target) return;
    pw_osk_draw_tiled(&g_pw_osk, target, 1920, 1080);
}
