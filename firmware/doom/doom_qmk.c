#include QMK_KEYBOARD_H
#include "doom_qmk.h"

#include "d_event.h"
#include "d_main.h"
#include "doomdef.h"
#include "m_argv.h"

extern void multicore_reset_core1(void);
extern void multicore_launch_core1_with_stack(void (*entry)(void), uint32_t *stack_bottom, size_t stack_size_bytes);

#ifndef OLED_ENABLE
#error "DOOM_QMK requires OLED_ENABLE"
#endif

enum {
    DOOM_W = 128,
    DOOM_H = 32,
    DOOM_FB_SIZE = (DOOM_W * DOOM_H) / 8,
    DOOM_EVENT_QUEUE_SIZE = 32,
};

extern const uint8_t doom_wad_data[];
extern const size_t doom_wad_size_bytes;

typedef struct {
    uint8_t type;
    uint8_t data1;
    int16_t data2;
    int16_t data3;
} doom_event_wire_t;

static volatile bool doom_active;
static volatile bool doom_ready;
static volatile bool doom_engine_started;
static volatile bool doom_engine_running;
static volatile bool doom_engine_faulted;
static volatile bool doom_engine_stalled;
static volatile uint32_t doom_heartbeat_counter;

static volatile uint8_t doom_fb[DOOM_FB_SIZE];

static volatile uint8_t event_head;
static volatile uint8_t event_tail;
static doom_event_wire_t event_queue[DOOM_EVENT_QUEUE_SIZE];
static uint32_t doom_core1_stack[1024 / sizeof(uint32_t)];
static uint32_t doom_seen_heartbeat;
static uint32_t doom_heartbeat_seen_at_ms;

static void queue_event(uint8_t type, uint8_t data1, int16_t data2, int16_t data3) {
    uint8_t head = event_head;
    uint8_t next = (uint8_t)((head + 1u) % DOOM_EVENT_QUEUE_SIZE);
    if (next == event_tail) {
        event_tail = (uint8_t)((event_tail + 1u) % DOOM_EVENT_QUEUE_SIZE);
    }
    event_queue[head].type = type;
    event_queue[head].data1 = data1;
    event_queue[head].data2 = data2;
    event_queue[head].data3 = data3;
    event_head = next;
}

bool doom_qmk_pop_event(event_t *ev) {
    if (event_tail == event_head) {
        return false;
    }

    doom_event_wire_t queued = event_queue[event_tail];
    event_tail = (uint8_t)((event_tail + 1u) % DOOM_EVENT_QUEUE_SIZE);

    ev->type = (evtype_t)queued.type;
    ev->data1 = queued.data1;
    ev->data2 = queued.data2;
    ev->data3 = queued.data3;
    return true;
}

static void doom_core1_entry(void) {
    static char arg0[] = "doom_qmk";
    static char arg1[] = "-iwad";
    static char arg2[] = "doom1.wad";
    static char arg3[] = "-mono128x32";
    static char arg4[] = "-nosound";
    static char *argv[] = {arg0, arg1, arg2, arg3, arg4, NULL};

    myargc = 5;
    myargv = argv;

    doom_engine_running = true;
    doom_engine_faulted = false;
    doom_engine_stalled = false;
    doom_heartbeat_counter = 0;
    D_DoomMain();
    doom_engine_running = false;
    doom_engine_faulted = true;
}

void doom_qmk_present_1bpp(const uint8_t *src, size_t len) {
    if (len > DOOM_FB_SIZE) {
        len = DOOM_FB_SIZE;
    }
    memcpy((void *)doom_fb, src, len);
}

void doom_qmk_init(void) {
    doom_ready = doom_wad_size_bytes >= 4
        && doom_wad_data[0] == 'I'
        && doom_wad_data[1] == 'W'
        && doom_wad_data[2] == 'A'
        && doom_wad_data[3] == 'D';

    doom_active = false;
    doom_engine_started = false;
    doom_engine_running = false;
    doom_engine_faulted = false;
    doom_engine_stalled = false;
    doom_heartbeat_counter = 0;
    doom_seen_heartbeat = 0;
    doom_heartbeat_seen_at_ms = timer_read32();
    event_head = 0;
    event_tail = 0;
    memset((void *)doom_fb, 0, sizeof(doom_fb));
}

void doom_qmk_toggle(void) {
    if (!doom_ready) {
        return;
    }

    if (!doom_engine_started) {
        doom_engine_faulted = false;
        doom_engine_stalled = false;
        doom_heartbeat_counter = 0;
        doom_seen_heartbeat = 0;
        doom_heartbeat_seen_at_ms = timer_read32();
        multicore_reset_core1();
        multicore_launch_core1_with_stack(doom_core1_entry, doom_core1_stack, sizeof(doom_core1_stack));
        doom_engine_started = true;
    }

    doom_active = !doom_active;
}

void doom_qmk_set_active(bool active) {
    if (!doom_ready) {
        doom_active = false;
        return;
    }
    doom_active = active;
}

bool doom_qmk_is_active(void) {
    return doom_active;
}

bool doom_qmk_engine_running(void) {
    return doom_engine_running;
}

bool doom_qmk_engine_started(void) {
    return doom_engine_started;
}

void doom_qmk_note_tic(void) {
    doom_heartbeat_counter++;
}

bool doom_qmk_is_stalled(void) {
    return doom_engine_stalled;
}

bool doom_qmk_has_fault(void) {
    return doom_engine_faulted;
}

uint32_t doom_qmk_heartbeat(void) {
    return doom_heartbeat_counter;
}

size_t doom_qmk_wad_size(void) {
    return doom_wad_size_bytes;
}

void doom_qmk_set_key(uint16_t keycode, bool pressed) {
    uint8_t doom_key = 0;
    switch (keycode) {
        case KC_W:
            doom_key = KEY_UPARROW;
            break;
        case KC_S:
            doom_key = KEY_DOWNARROW;
            break;
        case KC_A:
            doom_key = KEY_LEFTARROW;
            break;
        case KC_D:
            doom_key = KEY_RIGHTARROW;
            break;
        case KC_ENT:
            doom_key = KEY_ENTER;
            break;
        case KC_ESC:
            doom_key = KEY_ESCAPE;
            break;
        case KC_B:
        case KC_SPC:
            doom_key = KEY_RCTRL;
            break;
        default:
            break;
    }

    if (!doom_key) {
        return;
    }

    queue_event(pressed ? ev_keydown : ev_keyup, doom_key, 0, 0);
}

void doom_qmk_task(void) {
    if (!doom_engine_started) {
        return;
    }

    if (!doom_engine_running) {
        doom_engine_faulted = true;
        doom_engine_stalled = false;
        return;
    }

    uint32_t now = timer_read32();
    uint32_t heartbeat = doom_heartbeat_counter;

    if (heartbeat != doom_seen_heartbeat) {
        doom_seen_heartbeat = heartbeat;
        doom_heartbeat_seen_at_ms = now;
        doom_engine_stalled = false;
        return;
    }

    if (timer_elapsed32(doom_heartbeat_seen_at_ms) > 1000) {
        doom_engine_stalled = true;
    }
}

bool doom_qmk_oled_task(void) {
    if (!doom_active) {
        return false;
    }

    oled_write_raw((const char *)doom_fb, sizeof(doom_fb));
    return true;
}

void doom_qmk_copy_framebuffer(uint8_t *dst, size_t len) {
    if (!dst || !len) {
        return;
    }
    if (len > DOOM_FB_SIZE) {
        len = DOOM_FB_SIZE;
    }
    memcpy(dst, (const void *)doom_fb, len);
}
