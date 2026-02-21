#include QMK_KEYBOARD_H
#include "tetris_qmk.h"

#ifndef OLED_ENABLE
#error "TETRIS_QMK requires OLED_ENABLE"
#endif

enum {
    OLED_W = 128,
    OLED_H = 32,
    OLED_FB_SIZE = (OLED_W * OLED_H) / 8,
    BW = 10,
    BH = 14,
    CELL = 2,
    OX = 2,
    OY = 2,
};

static bool active;
static bool key_left;
static bool key_right;
static bool key_down;
static bool key_drop;

static uint8_t fb[OLED_FB_SIZE];
static uint8_t board[BH][BW];

typedef struct {
    int8_t x;
    int8_t y;
} piece_t;

static piece_t piece;
static uint16_t score;
static uint16_t tick;

static inline void fb_clear(void) {
    memset(fb, 0, sizeof(fb));
}

static inline void fb_set(uint8_t x, uint8_t y, bool on) {
    if (x >= OLED_W || y >= OLED_H) {
        return;
    }
    uint16_t idx = x + (y / 8) * OLED_W;
    uint8_t m = 1u << (y % 8);
    if (on) {
        fb[idx] |= m;
    } else {
        fb[idx] &= (uint8_t)~m;
    }
}

static void fb_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on) {
    if (!w || !h) {
        return;
    }
    for (uint8_t dx = 0; dx < w; dx++) {
        fb_set((uint8_t)(x + dx), y, on);
        fb_set((uint8_t)(x + dx), (uint8_t)(y + h - 1), on);
    }
    for (uint8_t dy = 0; dy < h; dy++) {
        fb_set(x, (uint8_t)(y + dy), on);
        fb_set((uint8_t)(x + w - 1), (uint8_t)(y + dy), on);
    }
}

static void draw_cell(uint8_t bx, uint8_t by, bool on) {
    uint8_t px = (uint8_t)(OX + 1 + bx * CELL);
    uint8_t py = (uint8_t)(OY + 1 + by * CELL);
    for (uint8_t dy = 0; dy < CELL; dy++) {
        for (uint8_t dx = 0; dx < CELL; dx++) {
            fb_set((uint8_t)(px + dx), (uint8_t)(py + dy), on);
        }
    }
}

static bool collides(int8_t px, int8_t py) {
    for (uint8_t dy = 0; dy < 2; dy++) {
        for (uint8_t dx = 0; dx < 2; dx++) {
            int8_t x = (int8_t)(px + dx);
            int8_t y = (int8_t)(py + dy);
            if (x < 0 || x >= BW || y < 0 || y >= BH) {
                return true;
            }
            if (board[y][x]) {
                return true;
            }
        }
    }
    return false;
}

static void spawn_piece(void) {
    piece.x = BW / 2 - 1;
    piece.y = 0;
    if (collides(piece.x, piece.y)) {
        memset(board, 0, sizeof(board));
        score = 0;
    }
}

static void lock_piece(void) {
    for (uint8_t dy = 0; dy < 2; dy++) {
        for (uint8_t dx = 0; dx < 2; dx++) {
            int8_t x = (int8_t)(piece.x + dx);
            int8_t y = (int8_t)(piece.y + dy);
            if (x >= 0 && x < BW && y >= 0 && y < BH) {
                board[y][x] = 1;
            }
        }
    }
}

static void clear_lines(void) {
    for (int8_t y = BH - 1; y >= 0; y--) {
        bool full = true;
        for (uint8_t x = 0; x < BW; x++) {
            if (!board[y][x]) {
                full = false;
                break;
            }
        }
        if (!full) {
            continue;
        }
        score++;
        for (int8_t yy = y; yy > 0; yy--) {
            memcpy(board[yy], board[yy - 1], BW);
        }
        memset(board[0], 0, BW);
        y++;
    }
}

void tetris_qmk_init(void) {
    active = false;
    key_left = key_right = key_down = key_drop = false;
    memset(board, 0, sizeof(board));
    score = 0;
    tick = 0;
    spawn_piece();
    fb_clear();
}

void tetris_qmk_set_active(bool on) {
    active = on;
}

void tetris_qmk_toggle(void) {
    active = !active;
}

bool tetris_qmk_is_active(void) {
    return active;
}

void tetris_qmk_set_key(uint16_t keycode, bool pressed) {
    if (!active) {
        return;
    }
    switch (keycode) {
        case KC_A:
            key_left = pressed;
            break;
        case KC_D:
            key_right = pressed;
            break;
        case KC_S:
            key_down = pressed;
            break;
        case KC_SPC:
        case KC_B:
            key_drop = pressed;
            break;
    }
}

void tetris_qmk_task(void) {
    if (!active) {
        return;
    }

    if (key_left && !collides((int8_t)(piece.x - 1), piece.y)) {
        piece.x--;
        key_left = false;
    }
    if (key_right && !collides((int8_t)(piece.x + 1), piece.y)) {
        piece.x++;
        key_right = false;
    }

    tick++;
    bool fall = key_drop || key_down || (tick % 18 == 0);
    if (fall) {
        if (!collides(piece.x, (int8_t)(piece.y + 1))) {
            piece.y++;
        } else {
            lock_piece();
            clear_lines();
            spawn_piece();
        }
        key_drop = false;
    }

    fb_clear();
    fb_rect(OX, OY, (uint8_t)(BW * CELL + 2), (uint8_t)(BH * CELL + 2), true);

    for (uint8_t y = 0; y < BH; y++) {
        for (uint8_t x = 0; x < BW; x++) {
            if (board[y][x]) {
                draw_cell(x, y, true);
            }
        }
    }

    for (uint8_t dy = 0; dy < 2; dy++) {
        for (uint8_t dx = 0; dx < 2; dx++) {
            draw_cell((uint8_t)(piece.x + dx), (uint8_t)(piece.y + dy), true);
        }
    }

    uint8_t bar = (uint8_t)(score % 20);
    for (uint8_t i = 0; i < bar; i++) {
        fb_set((uint8_t)(100 + i), 8, true);
    }
}

bool tetris_qmk_oled_task(void) {
    if (!active) {
        return false;
    }
    oled_write_raw((const char *)fb, sizeof(fb));
    return true;
}
