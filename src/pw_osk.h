/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PW_OSK_H
#define PW_OSK_H

#include <stdint.h>
#include <stddef.h>
#include "pw_controls.h"
#include "../ps5/src/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An on-screen virtual keyboard for the PS5 DualSense controller when no USB keyboard is plugged in.
 *
 * Trigger:
 *   When no physical keyboard is connected (keyboards == 0), pressing L3 + R3 together
 *   toggles the on-screen keyboard overlay.
 * Dismissal:
 *   Pressing Circle (PAD_CIRCLE) immediately closes/removes the keyboard.
 *   Pressing L3 + R3 again also closes the keyboard.
 *
 * Navigation:
 *   D-Pad (Up/Down/Left/Right) and Left Analog Stick navigate keys with wrap-around.
 * Actions:
 *   Cross (PAD_CROSS)      : types the focused key (or toggles Shift / sends Enter / Space / Bksp).
 *   Square (PAD_SQUARE)    : quick Backspace.
 *   Triangle (PAD_TRIANGLE): quick Space.
 *   L2 / R2                : quick Shift toggle.
 *   Circle (PAD_CIRCLE)    : closes/removes the keyboard.
 *
 * While active:
 *   Game controller inputs (XInput axes and buttons) and mouse cursor movement are masked
 *   so the player doesn't trigger in-game actions while typing.
 */

enum {
    PW_OSK_ROWS = 5,
    PW_OSK_MAX_COLS = 12,
    PW_OSK_DISPLAY_BLOCKS = 15
};

size_t pw_osk_tiled(unsigned x, unsigned y);

typedef struct PwOskKey {
    char ch_normal;
    char ch_shifted;
    const char *label;
    uint8_t vk;
    uint16_t scan;
    uint8_t span; /* columns covered (default 1) */
} PwOskKey;

typedef struct PwOsk {
    int active;
    int row;
    int col;
    int shift;
    uint32_t last_move_ms;
    uint32_t prev_held;
    int prev_trigger;
    uint8_t pending_up_vk;
    uint16_t pending_up_scan;
    uint8_t pending_up_shift;
} PwOsk;

extern PwOsk g_pw_osk;

void pw_osk_init(PwOsk *osk);
int pw_osk_is_active(const PwOsk *osk);
void pw_osk_open(PwOsk *osk);
void pw_osk_close(PwOsk *osk);

/* Updates OSK state from pad. Returns number of PwWineInput events queued into out. */
unsigned pw_osk_update(PwOsk *osk, const struct pad *pad, unsigned keyboards_connected,
                       PwWineInput *out, unsigned max_out, uint32_t now_ms);

/* Renders OSK onto linear RGBA/BGRA pixels (for tests/host). */
void pw_osk_draw_linear(const PwOsk *osk, uint32_t *pixels, unsigned width, unsigned height, unsigned pitch);

/* Renders OSK onto VideoOut's 64KB_R_X tiled PS5 display memory. */
void pw_osk_draw_tiled(const PwOsk *osk, uint8_t *buffer, unsigned width, unsigned height);

/* Global draw call used by display.present. If g_pw_osk is not active, returns immediately. */
void pw_osk_draw(uint8_t *target);

#ifdef __cplusplus
}
#endif

#endif
