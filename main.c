#include <gba.h>

extern const u8  frames_bin[];
extern const u32 frames_bin_size;
extern const u8  palette_bin[];
extern const u32 palette_bin_size;

#define FRAME_W     240
#define FRAME_H     160
#define FRAME_BYTES (FRAME_W * FRAME_H)

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

    dmaCopy(palette_bin, BG_PALETTE_MEM, palette_bin_size);

    u32 total_frames = frames_bin_size / FRAME_BYTES;
    u32 frame_idx = 0;
    int showing_page1 = 0;

    while (1) {
        VBlankIntrWait();

        const u8 *src = frames_bin + (frame_idx * FRAME_BYTES);
        u16 *back = showing_page1 ? VRAM_PAGE0 : VRAM_PAGE1;

        dmaCopy(src, back, FRAME_BYTES);

        if (showing_page1) {
            *REG_DISPCNT_PTR &= ~DISPCNT_BACKBUFFER;
        } else {
            *REG_DISPCNT_PTR |= DISPCNT_BACKBUFFER;
        }
        showing_page1 = !showing_page1;

        frame_idx++;
        if (frame_idx >= total_frames) {
            frame_idx = 0;
        }
    }

    return 0;
}
