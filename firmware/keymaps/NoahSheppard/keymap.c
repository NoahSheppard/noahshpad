// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layer_names {
    _BASE = 0, // binary 001
    _L000,     // binary 000
    _L010,     // binary 010
    _L011,     // binary 011
    _L100,     // binary 100
    _L101,     // binary 101
    _L110,     // binary 110
    _L111      // binary 111
};

enum custom_keycodes {
    BIN0 = SAFE_RANGE,
    BIN1,
    BIN2
};

static uint8_t bin_state = 0b001;

// index = binary state: 000,001,010,011,100,101,110,111
static const uint8_t bin_to_layer[8] = {
    _BASE, _BASE, _L010, _L011, _L100, _L101, _L110, _L111
};

static void apply_binary_layer(void) {
    if ((bin_state & 0x07) == 0) {
        bin_state = 0b001; // disallow "none selected", should not be possible?
    }
    layer_move(bin_to_layer[bin_state & 0x07]);
}

#ifdef RGBLIGHT_ENABLE
static void render_bin_indicators(void) {
    for (uint8_t i = 0; i < 3; i++) {
        if (bin_state & (1 << i)) {
            rgblight_setrgb_at(0xFF, 0xFF, 0xFF, i);
        } else {
            rgblight_setrgb_at(0x00, 0x00, 0x00, i);
        }
    }
}

static void render_wave_tail(void) {
    uint8_t base_hue = (timer_read() / 8) & 0xFF;
    for (uint8_t i = 3; i < RGBLIGHT_LED_COUNT; i++) {
        uint8_t hue = base_hue + ((i - 3)  * 10);
        rgblight_sethsv_at(hue, 255, 180, i);
    }
}
#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L000] = LAYOUT(
        BIN0, BIN1, BIN2, KC_1,
        KC_2, KC_3, KC_4, KC_5
    ),
    [_L010] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L011] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L100] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L101] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L110] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L111] = LAYOUT(
        BIN0, BIN1, BIN2, KC_A,
        KC_B, KC_C, KC_D, KC_E 
    ),
};

void keyboard_post_init_user(void) {
#ifdef RGBLIGHT_ENABLE
    rgblight_enable_noeeprom();
#endif
    apply_binary_layer();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case BIN0:
            bin_state ^= 0b001;
            apply_binary_layer();
            return false;
        case BIN1:
            bin_state ^= 0b010;
            apply_binary_layer();
            return false;
        case BIN2:
            bin_state ^= 0b100;
            apply_binary_layer();
            return false;
    }
    return true;
}

void housekeeping_task_user(void) {
#ifdef RGBLIGHT_ENABLE
    render_wave_tail();
    render_bin_indicators();
#endif
}
