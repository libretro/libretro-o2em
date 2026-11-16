#include <stdio.h>
#include <assert.h>
#include "../src/vpp.c"
struct resource app_data;

int main(void)
{
    static uint8_t pixels[BMPW * BMPH];
    const int decoder[8] = {1,16,4,64,2,32,8,128};
    int x, y, color, selected;
    assert(init_vpp());
    /* An enabled service row must disappear when R3 is cleared. */
    vpp_r = 9;
    for (x=0; x<40; x++) {
        vpp_mem[x][31][0] = 0x7f;
        vpp_mem[x][31][1] = 7;
    }
    vpp_update_screen();
    memset(vppbmp->line, 7, 3200);
    vpp_r = 1;
    vpp_update_screen();
    for (x=0; x<3200; x++) assert(vppbmp->line[x] == 0);
    /* Every decoder input, both luminance variants, at all four borders
       and in the active window. No color-specific exception is allowed. */
    vpp_r = 0;
    for (selected=0; selected<8; selected++) {
        TraReg = (uint8_t)(255 ^ decoder[selected]);
        for (color=0; color<16; color++) {
            memset(pixels, color, sizeof(pixels));
            need_update = 1;
            vpp_finish_bmp(pixels,9,5,331,245,BMPW,BMPH);
            for (y=0; y<BMPH; y++) for (x=0; x<BMPW; x++)
                assert(pixels[y*BMPW+x] == (((color&7)==selected) ? 0 : color));
        }
    }
    TraReg = 255;
    memset(pixels, 5, sizeof(pixels));
    vpp_finish_bmp(pixels,9,5,331,245,BMPW,BMPH);
    for (x=0; x<BMPW*BMPH; x++) assert(pixels[x] == 5);
    close_vpp();
    puts("PASS: disabled service row; 128 decoder/color combinations across full frame; bypass.");
    return 0;
}

