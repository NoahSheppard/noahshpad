#include <stdbool.h>
#include <string.h>

#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "m_argv.h"
#include "v_video.h"

#include "doom_qmk.h"

enum {
    OLED_W = 128,
    OLED_H = 32,
    OLED_FB_SIZE = (OLED_W * OLED_H) / 8,
    MONO_THRESHOLD = 96,
};

static byte local_palette[256 * 3];
static byte mono_luma[256];
static byte mono_fb[OLED_FB_SIZE];

extern void D_PostEvent(event_t *ev);
extern bool doom_qmk_pop_event(event_t *ev);

static void update_mono_luma(void) {
    for (int i = 0; i < 256; i++) {
        int r = gammatable[usegamma][local_palette[i * 3 + 0]];
        int g = gammatable[usegamma][local_palette[i * 3 + 1]];
        int b = gammatable[usegamma][local_palette[i * 3 + 2]];
        mono_luma[i] = (byte)((30 * r + 59 * g + 11 * b) / 100);
    }
}

static inline void mono_set(uint8_t x, uint8_t y, bool on) {
    uint16_t index = x + (y / 8) * OLED_W;
    uint8_t mask = 1u << (y % 8);
    if (on) {
        mono_fb[index] |= mask;
    } else {
        mono_fb[index] &= (uint8_t)~mask;
    }
}

static void convert_320x200_to_128x32_1bpp(const byte *src) {
    memset(mono_fb, 0, sizeof(mono_fb));

    for (int oy = 0; oy < OLED_H; oy++) {
        int sy0 = (oy * SCREENHEIGHT) / OLED_H;
        int sy1 = ((oy + 1) * SCREENHEIGHT) / OLED_H;
        if (sy1 <= sy0) {
            sy1 = sy0 + 1;
        }

        for (int ox = 0; ox < OLED_W; ox++) {
            int sx0 = (ox * SCREENWIDTH) / OLED_W;
            int sx1 = ((ox + 1) * SCREENWIDTH) / OLED_W;
            if (sx1 <= sx0) {
                sx1 = sx0 + 1;
            }

            int sum = 0;
            int count = 0;
            for (int sy = sy0; sy < sy1; sy++) {
                int row = sy * SCREENWIDTH;
                for (int sx = sx0; sx < sx1; sx++) {
                    sum += mono_luma[src[row + sx]];
                    count++;
                }
            }

            int avg = count ? (sum / count) : 0;
            mono_set((uint8_t)ox, (uint8_t)oy, avg >= MONO_THRESHOLD);
        }
    }

    doom_qmk_present_1bpp(mono_fb, sizeof(mono_fb));
}

void I_InitGraphics(void) {
    static bool inited = false;
    if (inited) {
        return;
    }

    inited = true;

    screens[0] = I_AllocLow(SCREENWIDTH * SCREENHEIGHT * 4);
    screens[1] = (byte *)screens[0] + SCREENWIDTH * SCREENHEIGHT;
    screens[2] = (byte *)screens[1] + SCREENWIDTH * SCREENHEIGHT;
    screens[3] = (byte *)screens[2] + SCREENWIDTH * SCREENHEIGHT;
    screens[4] = (byte *)screens[3] + SCREENWIDTH * SCREENHEIGHT;

    memset(local_palette, 0, sizeof(local_palette));
    update_mono_luma();
}

void I_ShutdownGraphics(void) {
}

void I_SetPalette(byte *palette) {
    memcpy(local_palette, palette, sizeof(local_palette));
    update_mono_luma();
}

void I_UpdateNoBlit(void) {
}

void I_FinishUpdate(void) {
    convert_320x200_to_128x32_1bpp(screens[0]);
}

void I_ReadScreen(byte *scr) {
    memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

void I_StartFrame(void) {
}

void I_StartTic(void) {
    doom_qmk_note_tic();

    event_t ev;
    while (doom_qmk_pop_event(&ev)) {
        D_PostEvent(&ev);
    }
}
