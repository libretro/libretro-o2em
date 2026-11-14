/* SPDX-License-Identifier: Artistic-2.0 */
/* Copyright (c) 2026 Bengt-Ove Peltz / O2EM-NG. */
#include "c7010_pal.h"

static long long C7010Pal_NormalizePhase(long long phase)
{
    phase %= C7010_PAL_FRAME_PIXELS;
    if (phase < 0)
        phase += C7010_PAL_FRAME_PIXELS;
    return phase;
}

void C7010Pal_Compute(unsigned long clk_counter, C7010PalPhase *out)
{
    long long phase;
    int line;
    int pixel;
    int vblank;
    int hblank;

    if (!out)
        return;

    phase = C7010Pal_NormalizePhase(
        (long long)clk_counter * C7010_PAL_PIXELS_PER_CYCLE +
        C7010_PAL_PHASE_INIT);

    line = (int)(phase / C7010_PAL_PIXELS_PER_LINE);
    pixel = (int)(phase % C7010_PAL_PIXELS_PER_LINE);

    /*
     * MAME 0.289 i8244 PAL:
     *   vblank: (v==242 && h>=365) || (v==312 && h<=365) || (242<v<312)
     *   hblank: 366 <= h < 454
     */
    vblank = (line == 242 && pixel >= 365) ||
             (line == 312 && pixel <= 365) ||
             (line > 242 && line < 312);
    hblank = (pixel >= 366 && pixel < 454);

    out->phase_pixels = phase;
    out->line = line;
    out->pixel = pixel;
    out->hblank = hblank;
    out->vblank = vblank;
    out->t1 = (hblank || vblank) ? 1 : 0;
}

int C7010Pal_BeamLineFromMaster(int master_clk)
{
    long long line;

    if (master_clk < 0)
        master_clk = 0;
    line = ((long long)master_clk * C7010_PAL_PIXELS_PER_CYCLE + 228LL) /
           C7010_PAL_PIXELS_PER_LINE;
    if (line < 0)
        line = 0;
    if (line >= C7010_PAL_LINES_PER_FRAME)
        line = C7010_PAL_LINES_PER_FRAME - 1;
    return (int)line;
}

int C7010Pal_FrameCycles(int frame)
{
    /*
     * 313*456/18 = 7929 + 1/3 machine cycles.  Integer schedule:
     * 7929, 7929, 7930, ... (average 7929.333), no floating point.
     */
    if (frame < 0)
        frame = 0;
    return C7010_PAL_FRAME_BASE + ((frame % 3) == 2 ? 1 : 0);
}

int C7010Pal_IsPalMachine(int c7010_active, int euro)
{
    return (c7010_active && euro) ? 1 : 0;
}
