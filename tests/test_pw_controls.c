/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../src/pw_controls.h"
#include "../src/proton_dualsense.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    pw_controls controls, loaded;
    pw_controls_mouse mouse = {0};
    PwWineInput events[4];
    struct pad pad = {.connected = 1, .touch_count = 1, .touch_id = 7, .touch_x = 100, .touch_y = 100};
    pw_controls_defaults(&controls);
    assert(controls.button[0] == 1 && !controls.button[1] && !controls.button[2]);
    assert(!pw_controls_assign(&controls, 0, 7)); /* R1 */
    assert(!pw_controls_assign(&controls, 1, 6)); /* L1 */
    assert(!pw_controls_assign(&controls, 2, 9)); /* R2 */
    assert(pw_controls_assign(&controls, 3, 1) == EINVAL);
    assert(pw_controls_assign(&controls, 0, PW_CONTROLS_BUTTONS) == EINVAL);
    assert(!pw_controls_update(&controls, &mouse, &pad, events));
    pad.held = PAD_R1 | PAD_L1 | PAD_CROSS;
    pad.touch_x += 10;
    pad.r2 = 0.6f;
    pad.raw_axes[5] = 200;
    assert(pw_controls_update(&controls, &mouse, &pad, events) == 4);
    assert(events[0].type == PW_WINE_INPUT_MOUSE_MOVE && events[0].x == 10);
    for (unsigned i = 0; i < 3; ++i) assert(events[i+1].code == i && events[i+1].down);
    assert(!pw_controls_update(&controls, &mouse, &pad, events));
    struct pad gamepad;
    uint8_t report[64];
    pw_controls_gamepad(&controls, &mouse, &pad, &gamepad);
    assert(gamepad.held == PAD_CROSS && !gamepad.r2 && !gamepad.raw_axes[5]);
    assert(gamepad.touch_x == pad.touch_x);
    proton_dualsense_report(&gamepad, 0, report);
    assert(!(report[9] & 0x0a)); /* R1 and R2 removed from HID too. */
    pad.held |= PAD_INTERCEPTED;
    assert(pw_controls_update(&controls, &mouse, &pad, events) == 3);
    for (unsigned i = 0; i < 3; ++i) assert(events[i].code == i && !events[i].down);
    pad.held = PAD_R1;
    pad.connected = 0;
    assert(!pw_controls_update(&controls, &mouse, &pad, events));
    assert(!pw_controls_assign(&controls, 1, 7));
    assert(!controls.button[0] && controls.button[1] == 7); /* conflict is unassigned */

    /* Defaults: a touchpad click with two fingers on the pad is the right button; with one, the left. */
    pw_controls defaults;
    pw_controls_mouse trackpad = {0};
    pw_controls_defaults(&defaults);
    struct pad touch = {.connected = 1, .touch_count = 2, .touch_id = 4, .touch_x = 300, .touch_y = 300};
    assert(!pw_controls_update(&defaults, &trackpad, &touch, events));
    touch.held = PAD_TOUCH_PAD;
    assert(pw_controls_update(&defaults, &trackpad, &touch, events) == 1 && events[0].code == 1 && events[0].down);
    touch.held = 0;
    assert(pw_controls_update(&defaults, &trackpad, &touch, events) == 1 && events[0].code == 1 && !events[0].down);
    touch.touch_count = 1;
    touch.held = PAD_TOUCH_PAD;
    assert(pw_controls_update(&defaults, &trackpad, &touch, events) == 1 && events[0].code == 0 && events[0].down);

    char folder[] = "/tmp/proton-controls-XXXXXX";
    assert(mkdtemp(folder));
    char path[256];
    snprintf(path, sizeof(path), "%s/controls.txt", folder);
    assert(pw_controls_load(&loaded, path) == ENOENT && loaded.button[0] == 1);
    assert(!pw_controls_save(&controls, path));
    assert(!pw_controls_load(&loaded, path) && !memcmp(&loaded, &controls, sizeof(controls)));
    controls.button[0] = 7; /* invalid duplicate must not damage the last saved file */
    assert(pw_controls_save(&controls, path) == EINVAL);
    assert(!pw_controls_load(&controls, path) && !memcmp(&loaded, &controls, sizeof(controls)));
    FILE *file = fopen(path, "w");
    assert(file);
    fputs("left cross\nright cross\nmiddle none\n", file);
    fclose(file);
    assert(pw_controls_load(&loaded, path) == EINVAL && loaded.button[0] == 1);
    file = fopen(path, "w");
    assert(file);
    fputs("left no-such-button\n", file);
    fclose(file);
    assert(pw_controls_load(&loaded, path) == EINVAL && loaded.button[0] == 1);
    unlink(path);
    rmdir(folder);

    /* Left stick pointer mode and Cross click testing: */
    pw_controls stick_ctrl;
    pw_controls_mouse stick_mouse = {0};
    pw_controls_defaults(&stick_ctrl);
    stick_ctrl.pointer_mode = PW_POINTER_LEFT_STICK;
    assert(!pw_controls_assign(&stick_ctrl, 0, 2)); /* button 0 (left click) = Cross */

    struct pad stick_pad = {.connected = 1, .left_x = 0.5f, .left_y = -0.5f};

    /* 1. In default Console Mode: no mouse movement, gamepad stick retained */
    assert(stick_mouse.mouse_active == 0);
    unsigned stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 0);
    struct pad stick_gamepad;
    pw_controls_gamepad(&stick_ctrl, &stick_mouse, &stick_pad, &stick_gamepad);
    assert(stick_gamepad.left_x == 0.5f && stick_gamepad.left_y == -0.5f);

    /* 2. Toggle to PC Mouse Mode with L1 + Cross */
    stick_pad.held = PAD_L1 | PAD_CROSS;
    pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_mouse.mouse_active == 1);
    stick_pad.held = 0;
    pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);

    /* In PC Mouse Mode: stick motion generates MOUSE_MOVE */
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 1);
    assert(events[0].type == PW_WINE_INPUT_MOUSE_MOVE);
    assert(events[0].x > 0 && events[0].y < 0);

    /* Gamepad left stick is zeroed when pointer mode is left stick and mouse is active */
    pw_controls_gamepad(&stick_ctrl, &stick_mouse, &stick_pad, &stick_gamepad);
    assert(stick_gamepad.left_x == 0.0f && stick_gamepad.left_y == 0.0f);
    assert(stick_gamepad.raw_axes[0] == 128 && stick_gamepad.raw_axes[1] == 128);

    /* Click with Cross: button 0 down */
    stick_pad.held = PAD_CROSS;
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count >= 1);
    int found_click_down = 0;
    for (unsigned i = 0; i < stick_count; ++i) {
        if (events[i].type == PW_WINE_INPUT_MOUSE_BUTTON && events[i].code == 0 && events[i].down)
            found_click_down = 1;
    }
    assert(found_click_down);

    /* Double click test: small jitter while button is pressed is pinned (cursor does not move) */
    stick_pad.left_x += 0.02f;
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 0); /* motion held back due to pinning */

    /* Release Cross: button 0 up */
    stick_pad.held = 0;
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 1 && events[0].type == PW_WINE_INPUT_MOUSE_BUTTON && events[0].code == 0 && !events[0].down);

    /* Second press of Cross (double click): still pinned, no motion event */
    stick_pad.held = PAD_CROSS;
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 1 && events[0].type == PW_WINE_INPUT_MOUSE_BUTTON && events[0].code == 0 && events[0].down);

    /* Release Cross again */
    stick_pad.held = 0;
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 1 && events[0].type == PW_WINE_INPUT_MOUSE_BUTTON && events[0].code == 0 && !events[0].down);

    /* 3. Toggle back to Console Mode with L1 + Cross */
    stick_pad.held = PAD_L1 | PAD_CROSS;
    pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_mouse.mouse_active == 0);
    stick_pad.held = 0;
    pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);

    /* Now in Console Mode again: stick generates no mouse move */
    stick_count = pw_controls_update(&stick_ctrl, &stick_mouse, &stick_pad, events);
    assert(stick_count == 0);
    pw_controls_gamepad(&stick_ctrl, &stick_mouse, &stick_pad, &stick_gamepad);
    assert(stick_gamepad.left_x == stick_pad.left_x);

    /* Test pointer mode persistence */
    char stick_folder[] = "/tmp/proton-controls-stick-XXXXXX";
    assert(mkdtemp(stick_folder));
    snprintf(path, sizeof(path), "%s/controls.txt", stick_folder);
    assert(!pw_controls_save(&stick_ctrl, path));
    assert(!pw_controls_load(&loaded, path));
    assert(loaded.pointer_mode == PW_POINTER_LEFT_STICK);
    assert(loaded.button[0] == 2);
    unlink(path);
    rmdir(stick_folder);

    /* Test PROTON_CURSOR_STICK environment variable */
    setenv("PROTON_CURSOR_STICK", "1", 1);
    pw_controls env_ctrl;
    pw_controls_defaults(&env_ctrl);
    assert(env_ctrl.pointer_mode == PW_POINTER_LEFT_STICK);
    unsetenv("PROTON_CURSOR_STICK");

    puts("controls: persistence, conflicts, motion, three clicks, HID consumption, left stick cursor, and Cross click PASS");
}
