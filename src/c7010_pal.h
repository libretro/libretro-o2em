/* SPDX-License-Identifier: Artistic-2.0 */
/* Copyright (c) 2026 Bengt-Ove Peltz / O2EM-NG. */
#ifndef C7010_PAL_H
#define C7010_PAL_H

#include <stdint.h>

/*
 * C7010 PAL raster timing foundation.
 *
 * Clock domain: clk_counter and master_clk are 8048 machine cycles.
 * One machine cycle advances the i8244/i8245 beam by 18 pixels.
 * PAL geometry: 456 pixels/line, 313 lines/frame, 142728 pixels/frame.
 *
 * This module is pure calculation only.  It does not drive the live CPU or
 * renderer by itself; the C7010 integration calls it explicitly.
 */

#define C7010_PAL_PIXELS_PER_LINE  456
#define C7010_PAL_LINES_PER_FRAME  313
#define C7010_PAL_PIXELS_PER_CYCLE 18
#define C7010_PAL_FRAME_PIXELS     (456LL * 313LL)
#define C7010_PAL_PHASE_INIT       (-24LL)
#define C7010_PAL_FRAME_BASE       7929

typedef struct
{
    long long phase_pixels;
    int line;
    int pixel;
    int hblank;
    int vblank;
    int t1;
} C7010PalPhase;

/* Continuous phase derived from the serialized absolute cycle counter. */
void C7010Pal_Compute(unsigned long clk_counter, C7010PalPhase *out);

/* Frame-relative beam line for renderer writes (NG/MAME update-on-write). */
int C7010Pal_BeamLineFromMaster(int master_clk);

/* PAL frame length schedule: 7929 cycles, with one 7930 every third frame. */
int C7010Pal_FrameCycles(int frame);

/* True if this build is allowed to use the C7010 PAL timing path. */
int C7010Pal_IsPalMachine(int c7010_active, int euro);

#endif
