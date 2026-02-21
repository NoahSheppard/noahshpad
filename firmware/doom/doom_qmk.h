#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void doom_qmk_init(void);
void doom_qmk_task(void);
void doom_qmk_toggle(void);
void doom_qmk_set_active(bool active);
bool doom_qmk_is_active(void);
bool doom_qmk_oled_task(void);
void doom_qmk_set_key(uint16_t keycode, bool pressed);
void doom_qmk_copy_framebuffer(uint8_t *dst, size_t len);
bool doom_qmk_engine_running(void);
bool doom_qmk_engine_started(void);
void doom_qmk_present_1bpp(const uint8_t *src, size_t len);
void doom_qmk_note_tic(void);
bool doom_qmk_is_stalled(void);
bool doom_qmk_has_fault(void);
uint32_t doom_qmk_heartbeat(void);

size_t doom_qmk_wad_size(void);
