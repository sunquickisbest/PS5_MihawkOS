/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pw_controls.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const struct { const char *id, *label; unsigned mask; } buttons[] = {
    {"none", "Unassigned", 0}, {"touchpad", "Touchpad", PAD_TOUCH_PAD},
    {"cross", "Cross", PAD_CROSS}, {"circle", "Circle", PAD_CIRCLE},
    {"square", "Square", PAD_SQUARE}, {"triangle", "Triangle", PAD_TRIANGLE},
    {"l1", "L1", PAD_L1}, {"r1", "R1", PAD_R1},
    {"l2", "L2", 0}, {"r2", "R2", 0},
    {"l3", "L3", PAD_L3}, {"r3", "R3", PAD_R3},
    {"up", "D-pad Up", PAD_UP}, {"down", "D-pad Down", PAD_DOWN},
    {"left", "D-pad Left", PAD_LEFT}, {"right", "D-pad Right", PAD_RIGHT},
    {"options", "Options", PAD_OPTIONS}
};
/* Keep the UI and parser on exactly the same table. */
_Static_assert(sizeof(buttons) / sizeof(buttons[0]) == PW_CONTROLS_BUTTONS, "button count");
static const char *actions[] = {"left", "right", "middle"};

const char *pw_controls_button_id(unsigned index)
{ return index < PW_CONTROLS_BUTTONS ? buttons[index].id : NULL; }
const char *pw_controls_button_label(unsigned index)
{ return index < PW_CONTROLS_BUTTONS ? buttons[index].label : NULL; }
const char *pw_controls_pointer_id(unsigned mode)
{ return mode == PW_POINTER_LEFT_STICK ? "left_stick" : "touchpad"; }
const char *pw_controls_pointer_label(unsigned mode)
{ return mode == PW_POINTER_LEFT_STICK ? "Left analog stick" : "Touchpad"; }

void pw_controls_defaults(pw_controls *controls)
{
    unsigned mode = PW_POINTER_TOUCHPAD;
    const char *stick_env = getenv("PROTON_CURSOR_STICK");
    if (!stick_env) stick_env = getenv("PROTON_POINTER_STICK");
    if (stick_env && (!strcmp(stick_env, "1") || !strcmp(stick_env, "left_stick") || !strcmp(stick_env, "true")))
        mode = PW_POINTER_LEFT_STICK;
    *controls = (pw_controls){.button = {1, 0, 0}, .pointer_mode = mode};
}

int pw_controls_assign(pw_controls *controls, unsigned mouse, unsigned button)
{
    if (mouse >= 3 || button >= PW_CONTROLS_BUTTONS) return EINVAL;
    for (unsigned i = 0; i < 3; ++i)
        if (button && controls->button[i] == button) controls->button[i] = 0;
    controls->button[mouse] = button;
    return 0;
}

int pw_controls_load(pw_controls *controls, const char *path)
{
    pw_controls parsed = {{0}, PW_POINTER_TOUCHPAD};
    char action[32], id[32];
    pw_controls_defaults(controls);
    FILE *file = fopen(path, "r");
    if (!file) return errno;
    int error = 0;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned index;
        if (fscanf(file, "%31s %31s", action, id) != 2 || strcmp(action, actions[i])) { error = EINVAL; break; }
        for (index = 0; index < PW_CONTROLS_BUTTONS; ++index)
            if (!strcmp(id, buttons[index].id)) break;
        if (index == PW_CONTROLS_BUTTONS) { error = EINVAL; break; }
        for (unsigned j = 0; j < i; ++j)
            if (index && parsed.button[j] == index) error = EINVAL;
        parsed.button[i] = index;
    }
    if (!error) {
        int r = fscanf(file, "%31s %31s", action, id);
        if (r == 2) {
            if (!strcmp(action, "pointer")) {
                if (!strcmp(id, "left_stick")) parsed.pointer_mode = PW_POINTER_LEFT_STICK;
                else if (!strcmp(id, "touchpad")) parsed.pointer_mode = PW_POINTER_TOUCHPAD;
                else error = EINVAL;
            } else {
                error = EINVAL;
            }
        } else if (r == 1) {
            error = EINVAL;
        }
    }
    if (!error && fscanf(file, "%31s", id) == 1) error = EINVAL;
    if (ferror(file)) error = EIO;
    fclose(file);
    if (!error) {
        const char *stick_env = getenv("PROTON_CURSOR_STICK");
        if (!stick_env) stick_env = getenv("PROTON_POINTER_STICK");
        if (stick_env && (!strcmp(stick_env, "1") || !strcmp(stick_env, "left_stick") || !strcmp(stick_env, "true")))
            parsed.pointer_mode = PW_POINTER_LEFT_STICK;
        const char *cross_env = getenv("PROTON_CLICK_CROSS");
        if (!cross_env) cross_env = getenv("PROTON_MOUSE_CROSS");
        if (cross_env && (!strcmp(cross_env, "1") || !strcmp(cross_env, "true")))
            parsed.button[0] = 2;
        *controls = parsed;
    }
    return error;
}

int pw_controls_save(const pw_controls *controls, const char *path)
{
    char temporary[1024];
    for (unsigned i = 0; i < 3; ++i) {
        if (controls->button[i] >= PW_CONTROLS_BUTTONS) return EINVAL;
        for (unsigned j = 0; j < i; ++j)
            if (controls->button[i] && controls->button[i] == controls->button[j]) return EINVAL;
    }
    if (controls->pointer_mode > PW_POINTER_LEFT_STICK) return EINVAL;
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary)) return ENAMETOOLONG;
    FILE *file = fopen(temporary, "w");
    if (!file) return errno;
    int error = 0;
    for (unsigned i = 0; i < 3; ++i)
        if (fprintf(file, "%s %s\n", actions[i], buttons[controls->button[i]].id) < 0) error = EIO;
    if (!error) {
        const char *p_id = controls->pointer_mode == PW_POINTER_LEFT_STICK ? "left_stick" : "touchpad";
        if (fprintf(file, "pointer %s\n", p_id) < 0) error = EIO;
    }
    if (fflush(file) || fsync(fileno(file))) error = errno;
    if (fclose(file)) error = errno;
    if (!error && rename(temporary, path)) error = errno;
    if (error) unlink(temporary);
    return error;
}

static int down(unsigned index, const struct pad *pad)
{
    if (!pad->connected || (pad->held & PAD_INTERCEPTED) || !index || index >= PW_CONTROLS_BUTTONS) return 0;
    if (index == 8) return pad->l2 >= 0.5f;
    if (index == 9) return pad->r2 >= 0.5f;
    return !!(pad->held & buttons[index].mask);
}

unsigned pw_controls_update(const pw_controls *controls, pw_controls_mouse *state,
                            const struct pad *pad, PwWineInput events[4])
{
    int connected = pad->connected && !(pad->held & PAD_INTERCEPTED);
    unsigned count = 0;

    if (controls->pointer_mode == PW_POINTER_LEFT_STICK) {
        /* L1 + Cross toggle combo: switch between Console Gamepad Mode and PC Mouse Mode */
        uint32_t toggle_combo = (PAD_L1 | PAD_CROSS);
        if ((pad->held & toggle_combo) == toggle_combo) {
            if (!state->toggle_held) {
                state->mouse_active = !state->mouse_active;
                state->toggle_held = 1;
                /* If switched to Console Mode, release mouse buttons and unpin */
                if (!state->mouse_active) {
                    if (state->stick_clicked) {
                        events[count++] = (PwWineInput){PW_WINE_INPUT_MOUSE_BUTTON, 0, 0, 0, 0};
                        state->stick_clicked = 0;
                    }
                }
                state->stick_pinned = 0;
            }
            return count;
        } else {
            state->toggle_held = 0;
        }

        /* When in Left Stick mode and mouse is inactive (default on game launch),
         * completely remove PC mouse options and stay console-only */
        if (!state->mouse_active) {
            return count;
        }

        float lx = connected ? pad->left_x : 0.0f;
        float ly = connected ? pad->left_y : 0.0f;
        if (lx > -0.05f && lx < 0.05f) lx = 0.0f;
        if (ly > -0.05f && ly < 0.05f) ly = 0.0f;
        float mag = sqrtf(lx * lx + ly * ly);
        if (mag > 1.0f) { lx /= mag; ly /= mag; mag = 1.0f; }

        float speed = mag * 16.0f;
        float move_x = lx * speed + state->stick_remainder_x;
        float move_y = ly * speed + state->stick_remainder_y;
        int dx = (int)move_x;
        int dy = (int)move_y;
        state->stick_remainder_x = move_x - (float)dx;
        state->stick_remainder_y = move_y - (float)dy;

        int clicked_0 = down(controls->button[0], pad);

        if (state->stick_pinned) {
            state->stick_held_x += dx;
            state->stick_held_y += dy;
            dx = dy = 0;
            if (state->stick_held_x > 20 || state->stick_held_x < -20 ||
                state->stick_held_y > 20 || state->stick_held_y < -20) {
                dx = state->stick_held_x;
                dy = state->stick_held_y;
                state->stick_pinned = 0;
            }
        }
        if (mag == 0.0f && !clicked_0) {
            state->stick_pinned = 0;
            state->stick_held_x = 0;
            state->stick_held_y = 0;
        }

        if (connected && (dx || dy))
            events[count++] = (PwWineInput){PW_WINE_INPUT_MOUSE_MOVE, 0, dx, dy, 0};

        if (clicked_0 != state->stick_clicked) {
            if (clicked_0) {
                state->stick_pinned = 1;
                state->stick_held_x = 0;
                state->stick_held_y = 0;
            }
            events[count++] = (PwWineInput){PW_WINE_INPUT_MOUSE_BUTTON, 0, 0, 0, (unsigned)clicked_0};
            state->stick_clicked = clicked_0;
        }
        if (!connected) state->stick_pinned = 0;

        int *previous[] = {&state->right, &state->middle};
        for (unsigned i = 1; i < 3; ++i) {
            int clicked = down(controls->button[i], pad);
            if (clicked != *previous[i-1]) {
                if (clicked) {
                    state->stick_pinned = 1;
                    state->stick_held_x = 0;
                    state->stick_held_y = 0;
                }
                events[count++] = (PwWineInput){PW_WINE_INPUT_MOUSE_BUTTON, i, 0, 0, (unsigned)clicked};
            }
            *previous[i-1] = clicked;
        }
        return count;
    }

    int clicked_0 = down(controls->button[0], pad);
    count = proton_touchpad_update(&state->pointer, connected, pad->touch_count != 0,
        pad->touch_id, pad->touch_x, pad->touch_y, clicked_0,
        controls->button[0] == 1 && pad->touch_count >= 2, events);

    int *previous[] = {&state->right, &state->middle};
    for (unsigned i = 1; i < 3; ++i) {
        int clicked = down(controls->button[i], pad);
        if (clicked != *previous[i-1]) {
            events[count++] = (PwWineInput){PW_WINE_INPUT_MOUSE_BUTTON, i, 0, 0, (unsigned)clicked};
        }
        *previous[i-1] = clicked;
    }
    return count;
}

void pw_controls_gamepad(const pw_controls *controls, const pw_controls_mouse *mouse,
                         const struct pad *input, struct pad *output)
{
    *output = *input;

    if (controls->pointer_mode == PW_POINTER_LEFT_STICK) {
        /* When L1 + Cross is being held to toggle modes, mask them so in-game actions aren't triggered */
        if ((input->held & (PAD_L1 | PAD_CROSS)) == (PAD_L1 | PAD_CROSS)) {
            output->held &= ~(PAD_L1 | PAD_CROSS);
            output->pressed &= ~(PAD_L1 | PAD_CROSS);
        }

        /* In pure Console Mode (default), do not mask any gamepad buttons or analog sticks */
        if (!mouse || !mouse->mouse_active) {
            return;
        }

        /* In PC Mouse Mode: mask mapped mouse buttons and stick axes */
        for (unsigned i = 0; i < 3; ++i) {
            unsigned index = controls->button[i];
            if (index >= PW_CONTROLS_BUTTONS) continue;
            output->held &= ~buttons[index].mask;
            output->pressed &= ~buttons[index].mask;
            if (index == 8) { output->l2 = 0; output->raw_axes[4] = 0; }
            if (index == 9) { output->r2 = 0; output->raw_axes[5] = 0; }
        }
        output->left_x = 0;
        output->left_y = 0;
        output->raw_axes[0] = 128;
        output->raw_axes[1] = 128;
        return;
    }

    /* Touchpad mode */
    for (unsigned i = 0; i < 3; ++i) {
        unsigned index = controls->button[i];
        if (index >= PW_CONTROLS_BUTTONS) continue;
        output->held &= ~buttons[index].mask;
        output->pressed &= ~buttons[index].mask;
        if (index == 8) { output->l2 = 0; output->raw_axes[4] = 0; }
        if (index == 9) { output->r2 = 0; output->raw_axes[5] = 0; }
    }
}
