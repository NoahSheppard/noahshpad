#pragma once

#include <stdbool.h>
#include <stdint.h>

void tetris_qmk_init(void);
void tetris_qmk_task(void);
void tetris_qmk_toggle(void);
void tetris_qmk_set_active(bool active);
bool tetris_qmk_is_active(void);
bool tetris_qmk_oled_task(void);
void tetris_qmk_set_key(uint16_t keycode, bool pressed);
