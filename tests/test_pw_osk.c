/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../src/pw_osk.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    PwOsk osk;
    pw_osk_init(&osk);
    assert(!pw_osk_is_active(&osk));
    assert(osk.row == 1 && osk.col == 0 && osk.shift == 0);

    struct pad pad = {0};
    PwWineInput events[4];

    /* 1. L3+R3 with keyboards_connected > 0 should NOT activate OSK */
    pad.held = PAD_L3 | PAD_R3;
    unsigned count = pw_osk_update(&osk, &pad, 1, events, 4, 100);
    assert(count == 0);
    assert(!pw_osk_is_active(&osk));

    /* Release combo */
    pad.held = 0;
    pw_osk_update(&osk, &pad, 1, events, 4, 116);
    assert(!pw_osk_is_active(&osk));

    /* 2. L3+R3 with keyboards_connected == 0 SHOULD activate OSK */
    pad.held = PAD_L3 | PAD_R3;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 132);
    assert(count == 0);
    assert(pw_osk_is_active(&osk));

    /* Release combo */
    pad.held = 0;
    pw_osk_update(&osk, &pad, 0, events, 4, 148);
    assert(pw_osk_is_active(&osk));

    /* 3. Circle button MUST remove / close the keyboard */
    pad.held = PAD_CIRCLE;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 164);
    assert(count == 0);
    assert(!pw_osk_is_active(&osk)); /* Closed! */

    /* Release Circle */
    pad.held = 0;
    pw_osk_update(&osk, &pad, 0, events, 4, 180);

    /* 4. Re-open with L3+R3 */
    pad.held = PAD_L3 | PAD_R3;
    pw_osk_update(&osk, &pad, 0, events, 4, 196);
    assert(pw_osk_is_active(&osk));
    pad.held = 0;
    pw_osk_update(&osk, &pad, 0, events, 4, 212);

    /* 5. Navigation tests */
    /* Start at row 1, col 0 ('Q') */
    assert(osk.row == 1 && osk.col == 0);

    /* Move Right with D-Pad */
    pad.held = PAD_RIGHT;
    pw_osk_update(&osk, &pad, 0, events, 4, 400);
    assert(osk.row == 1 && osk.col == 1); /* 'W' */
    pad.held = 0;
    pw_osk_update(&osk, &pad, 0, events, 4, 416);

    /* Move Left with Left Stick */
    pad.left_x = -0.8f;
    pw_osk_update(&osk, &pad, 0, events, 4, 600);
    assert(osk.row == 1 && osk.col == 0); /* Back to 'Q' */
    pad.left_x = 0.0f;
    pw_osk_update(&osk, &pad, 0, events, 4, 616);

    /* Left wrap around */
    pad.held = PAD_LEFT;
    pw_osk_update(&osk, &pad, 0, events, 4, 800);
    assert(osk.row == 1 && osk.col == 11); /* ']' */
    pad.held = 0;
    pw_osk_update(&osk, &pad, 0, events, 4, 816);

    /* Move Down to Row 2 */
    pad.held = PAD_DOWN;
    pw_osk_update(&osk, &pad, 0, events, 4, 1000);
    assert(osk.row == 2 && osk.col == 11); /* 'Enter' */
    pad.held = 0;
    pw_osk_update(&osk, &pad, 0, events, 4, 1016);

    /* 6. Typing with Cross on 'Enter' */
    pad.held = PAD_CROSS;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 1200);
    assert(count == 1);
    assert(events[0].type == PW_WINE_INPUT_KEY);
    assert((events[0].code & 0xff) == 0x0d); /* VK_RETURN */
    assert(events[0].down == 1);

    /* Next frame: key up must be sent */
    pad.held = 0;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 1216);
    assert(count == 1);
    assert(events[0].type == PW_WINE_INPUT_KEY);
    assert((events[0].code & 0xff) == 0x0d);
    assert(events[0].down == 0);

    /* 7. Quick buttons: Square = Backspace, Triangle = Space */
    pad.held = PAD_SQUARE;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 1300);
    assert(count == 1);
    assert((events[0].code & 0xff) == 0x08); /* VK_BACK */
    assert(events[0].down == 1);
    pad.held = 0;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 1316);
    assert(count == 1 && events[0].down == 0);

    pad.held = PAD_TRIANGLE;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 1400);
    assert(count == 1);
    assert((events[0].code & 0xff) == 0x20); /* VK_SPACE */
    assert(events[0].down == 1);
    pad.held = 0;
    count = pw_osk_update(&osk, &pad, 0, events, 4, 1416);
    assert(count == 1 && events[0].down == 0);

    /* 8. Quick trigger: L2 toggles Shift */
    assert(osk.shift == 0);
    pad.l2 = 0.9f;
    pw_osk_update(&osk, &pad, 0, events, 4, 1500);
    assert(osk.shift == 1);
    pad.l2 = 0.0f;
    pw_osk_update(&osk, &pad, 0, events, 4, 1516);
    assert(osk.shift == 1);

    /* 9. Rendering tests (linear and tiled) */
    uint32_t *linear = (uint32_t *)calloc(1920 * 1080, sizeof(uint32_t));
    assert(linear);
    pw_osk_draw_linear(&osk, linear, 1920, 1080, 1920 * sizeof(uint32_t));
    /* Verify that some pixels were drawn */
    int non_zero = 0;
    for (int i = 0; i < 1920 * 1080; ++i) if (linear[i] != 0) non_zero++;
    assert(non_zero > 1000);
    free(linear);

    /* Tiled buffer */
    uint8_t *tiled = (uint8_t *)calloc(PW_OSK_DISPLAY_BLOCKS * 9, 65536);
    assert(tiled);
    pw_osk_draw_tiled(&osk, tiled, 1920, 1080);
    non_zero = 0;
    for (size_t i = 0; i < (size_t)PW_OSK_DISPLAY_BLOCKS * 9 * 65536; ++i) if (tiled[i] != 0) non_zero++;
    assert(non_zero > 1000);
    free(tiled);

    /* 10. Close with Circle again */
    pad.held = PAD_CIRCLE;
    pw_osk_update(&osk, &pad, 0, events, 4, 1600);
    assert(!pw_osk_is_active(&osk));

    return 0;
}
