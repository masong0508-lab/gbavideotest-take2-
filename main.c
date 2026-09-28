#include <gba.h>

extern const u8 frames_bin[];
extern const u8 frames_bin_end[];
extern const u8 palette_bin[];
extern const u8 palette_bin_end[];

#define FRAME_W     240
#define FRAME_H     160
#define FRAME_BYTES (FRAME_W * FRAME_H)

// Length of one full playback of the clip, in seconds.
// Change this number to speed up / slow down the video.
#define CLIP_SECONDS 10
// GBA refreshes ~59.73 times per second
#define DURATION_VBLANKS ((CLIP_SECONDS * 5973) / 100)

#define REG_DISPCNT_PTR     ((vu16*)0x04000000)
#define DISPCNT_MODE4       0x0004
#define DISPCNT_BG2         0x0400
#define DISPCNT_BACKBUFFER  0x0010

#define VRAM_PAGE0      ((u16*)0x06000000)
#define VRAM_PAGE1      ((u16*)0x0600A000)
#define BG_PALETTE_MEM  ((u16*)0x05000000)

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    *REG_DISPCNT_PTR = DISPCNT_MODE4 | DISPCNT_BG2;

    u32 frames_size  = (u32)(frames_bin_end - frames_bin);
    u32 palette_size = (u32)(palette_bin_end - palette_bin);
    if (palette_size > 512) palette_size = 512;

    dmaCopy(palette_bin, BG_PALETTE_MEM, palette_size);

    u32 total_frames = frames_size / FRAME_BYTES;
    if (total_frames == 0) {
        while (1) VBlankIntrWait();
    }

    u32 tick = 0;
    u32 last_idx = 0xFFFFFFFF;
    int showing_page1 = 0;

    while (1) {
        VBlankIntrWait();

        // Map elapsed time onto the clip so it lasts CLIP_SECONDS total.
        u32 frame_idx = (tick * total_frames) / DURATION_VBLANKS;
        if (frame_idx >= total_frames) frame_idx = total_frames - 1;

        // Only draw when the video frame actually changes.
        if (frame_idx != last_idx) {
            const u8 *src = frames_bin + (frame_idx * FRAME_BYTES);
            u16 *back = showing_page1 ? VRAM_PAGE0 : VRAM_PAGE1;

            dmaCopy(src, back, FRAME_BYTES);

            if (showing_page1) {
                *REG_DISPCNT_PTR &= ~DISPCNT_BACKBUFFER;
            } else {
                *REG_DISPCNT_PTR |= DISPCNT_BACKBUFFER;
            }
            showing_page1 = !showing_page1;
            last_idx = frame_idx;
        }

        tick++;
        if (tick >= DURATION_VBLANKS) {
            tick = 0;
        }
    }

    return 0;
}
