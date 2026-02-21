// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "doom/doom_qmk.h"
#include "tetris/tetris_qmk.h"

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
    BIN2,
    DOOM_TOG,
    TETRIS_TOG
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

#ifdef OLED_ENABLE
enum {
    OLED_W = 128,
    OLED_H = 32,
    OLED_FB_SIZE = (OLED_W * OLED_H) / 8,
};

static uint8_t oled_fb[OLED_FB_SIZE];

static inline void fb_clear(void) { memset(oled_fb, 0, sizeof(oled_fb)); }

static inline void fb_set_pixel(uint8_t x, uint8_t y, bool on) {
    if (x >= OLED_W || y >= OLED_H) return;
    uint16_t index = x + (y / 8) * OLED_W;
    uint8_t mask = 1 << (y % 8);
    if (on) oled_fb[index] |= mask;
    else    oled_fb[index] &= ~mask;
}

static void fb_hline(uint8_t x, uint8_t y, uint8_t width, bool on) {
    for (uint8_t column = 0; column < width; column++) {
        fb_set_pixel(x + column, y, on);
    }
}

static void fb_vline(uint8_t x, uint8_t y, uint8_t height, bool on) {
    for (uint8_t row = 0; row < height; row++) {
        fb_set_pixel(x, y + row, on);
    }
}

static void fb_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool on) {
    if (width == 0 || height == 0) return;
    fb_hline(x, y, width, on);
    fb_hline(x, y + height - 1, width, on);
    fb_vline(x, y, height, on);
    fb_vline(x + width - 1, y, height, on);
}

static void render_oled_pixels(void) {
    fb_clear();

    fb_rect(0, 0, OLED_W, OLED_H, true);

    for (uint8_t bit = 0; bit < 3; bit++) {
        uint8_t x = 6 + (bit * 8);
        uint8_t y_bottom = OLED_H - 3;
        uint8_t bar_height = (bin_state & (1 << bit)) ? 12 : 4;
        for (uint8_t row = 0; row < bar_height; row++) {
            fb_hline(x, y_bottom - row, 5, true);
        }
    }

    uint8_t scan_x = (timer_read32() / 20) % OLED_W;
    fb_vline(scan_x, 1, OLED_H - 2, true);
}

static void render_doom_status_overlay(void) {
    uint8_t base_x = OLED_W - 5;
    uint8_t base_y = 1;

    if (doom_qmk_has_fault()) {
        for (uint8_t i = 0; i < 4; i++) {
            fb_set_pixel(base_x + i, base_y + i, true);
            fb_set_pixel(base_x + (3 - i), base_y + i, true);
        }
        return;
    }

    if (doom_qmk_is_stalled()) {
        fb_rect(base_x, base_y, 4, 4, true);
        return;
    }

    if (doom_qmk_engine_running()) {
        bool pulse = (doom_qmk_heartbeat() & 0x08u) != 0;
        if (pulse) {
            for (uint8_t y = 0; y < 4; y++) {
                for (uint8_t x = 0; x < 4; x++) {
                    fb_set_pixel(base_x + x, base_y + y, true);
                }
            }
        } else {
            fb_rect(base_x, base_y, 4, 4, true);
        }
    }
}

bool oled_task_user(void) {
    if (tetris_qmk_oled_task()) {
        return false;
    }

    if (doom_qmk_is_active()) {
        doom_qmk_copy_framebuffer(oled_fb, sizeof(oled_fb));
        render_doom_status_overlay();
        oled_write_raw((const char *)oled_fb, sizeof(oled_fb));
        return false;
    }

    render_oled_pixels();
    oled_write_raw((const char *)oled_fb, sizeof(oled_fb));
    return false; 
}
#endif

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
        BIN0, BIN1, BIN2, DOOM_TOG,
        KC_B, KC_C, KC_D, KC_E 
    ),
    [_L000] = LAYOUT(
        BIN0, BIN1, BIN2, TETRIS_TOG,
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
    doom_qmk_init();
    tetris_qmk_init();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    doom_qmk_set_key(keycode, record->event.pressed);
    tetris_qmk_set_key(keycode, record->event.pressed);

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
        case DOOM_TOG:
            doom_qmk_toggle();
            if (doom_qmk_is_active()) {
                tetris_qmk_set_active(false);
            }
            return false;
        case TETRIS_TOG:
            tetris_qmk_toggle();
            if (tetris_qmk_is_active()) {
                doom_qmk_set_active(false);
            }
            return false;
    }
    return true;
}

void housekeeping_task_user(void) {
    tetris_qmk_task();
    doom_qmk_task();

#ifdef RGBLIGHT_ENABLE
    render_wave_tail();
    render_bin_indicators();
#endif
}
