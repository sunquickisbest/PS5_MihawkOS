/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include "proton_touchpad.h"
#include "../ps5/src/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One global mapping, read by the launcher and each owner before game startup.
 * Stable button names persist atomically. A button owns at most one mouse
 * action; assigning it again removes its previous assignment. Mapped buttons
 * are removed from XInput/HID, while touch motion remains available.
 * Disconnect/shell interception releases every held mouse button.
 * File functions return errno values; failed reads leave the defaults and
 * failed saves leave the previous file. The caller owns the parent folder.
 *
 * Pointer mode: PW_POINTER_TOUCHPAD (0, the default) drives the cursor from
 * the DualSense touchpad.  PW_POINTER_LEFT_STICK (1) drives it from the
 * left analog stick with a quadratic acceleration curve; the stick's axes
 * are then zeroed from the XInput/HID gamepad so the game sees no phantom
 * movement.  The mode is persisted with the button assignments. */
enum { PW_CONTROLS_BUTTONS = 17, PW_CONTROLS_MOUSE_BUTTONS = 3 };
enum { PW_POINTER_TOUCHPAD = 0, PW_POINTER_LEFT_STICK = 1 };
typedef struct pw_controls { unsigned button[3]; unsigned pointer_mode; } pw_controls;
typedef struct pw_controls_mouse {
    ProtonTouchpad pointer;
    int right, middle;
    /* Left-stick cursor: sub-pixel remainder carried across ticks. */
    float stick_remainder_x, stick_remainder_y;
    /* Pinning for the stick cursor (same idea as the touchpad's): a click
     * freezes the pointer so a double-press of Cross lands at the same
     * pixel and Windows sees a double-click.  held_x/y accumulate motion
     * while pinned; the pin breaks when the total exceeds the threshold. */
    int stick_pinned, stick_held_x, stick_held_y;
    int stick_clicked;
    /* PC Mouse Mode vs Console Gamepad Mode:
     * 0 = pure console gamepad mode (default at game startup)
     * 1 = PC mouse mode (activated via L1 + Cross toggle) */
    int mouse_active;
    int toggle_held;
} pw_controls_mouse;

const char *pw_controls_button_id(unsigned index);
const char *pw_controls_button_label(unsigned index);
const char *pw_controls_pointer_id(unsigned mode);
const char *pw_controls_pointer_label(unsigned mode);
void pw_controls_defaults(pw_controls *controls);
int pw_controls_assign(pw_controls *controls, unsigned mouse, unsigned button);
int pw_controls_load(pw_controls *controls, const char *path);
int pw_controls_save(const pw_controls *controls, const char *path);
unsigned pw_controls_update(const pw_controls *controls, pw_controls_mouse *state,
                            const struct pad *pad, PwWineInput events[4]);
void pw_controls_gamepad(const pw_controls *controls, const pw_controls_mouse *mouse,
                         const struct pad *input, struct pad *output);

#ifdef __cplusplus
}
#endif
