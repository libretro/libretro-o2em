/* SPDX-License-Identifier: Artistic-2.0 */
/* Copyright (c) 2026 Bengt-Ove Peltz / O2EM-NG. */
#include <stdio.h>
#include <stdlib.h>
#include "c7010_pal.h"

static int fails;

static void expect_int(const char *what, int got, int want)
{
    if (got != want)
    {
        printf("FAIL %s got=%d want=%d\n", what, got, want);
        ++fails;
    }
    else
    {
        printf("PASS %s = %d\n", what, got);
    }
}

static void expect_ll(const char *what, long long got, long long want)
{
    if (got != want)
    {
        printf("FAIL %s got=%lld want=%lld\n", what, got, want);
        ++fails;
    }
    else
    {
        printf("PASS %s = %lld\n", what, got);
    }
}

int main(void)
{
    C7010PalPhase p;
    long long sum = 0;
    long long prev;
    int i;

    C7010Pal_Compute(0, &p);
    expect_ll("phase(0)", p.phase_pixels, 142704);
    expect_int("line(0)", p.line, 312);
    expect_int("pixel(0)", p.pixel, 432);
    expect_int("hblank(0)", p.hblank, 1);
    expect_int("vblank(0)", p.vblank, 0);
    expect_int("t1(0)", p.t1, 1);

    C7010Pal_Compute(1, &p);
    expect_ll("phase(1)", p.phase_pixels, 142722);
    expect_int("line(1)", p.line, 312);
    expect_int("pixel(1)", p.pixel, 450);
    expect_int("hblank(1)", p.hblank, 1);
    expect_int("t1(1)", p.t1, 1);

    C7010Pal_Compute(2, &p);
    expect_ll("phase(2)", p.phase_pixels, 12);
    expect_int("line(2)", p.line, 0);
    expect_int("pixel(2)", p.pixel, 12);
    expect_int("hblank(2)", p.hblank, 0);
    expect_int("t1(2) falling edge", p.t1, 0);

    C7010Pal_Compute(6151, &p);
    expect_int("line(6151)", p.line, 242);
    expect_int("pixel(6151)", p.pixel, 342);
    expect_int("vblank(6151)", p.vblank, 0);
    expect_int("t1(6151)", p.t1, 0);

    C7010Pal_Compute(6153, &p);
    expect_int("line(6153)", p.line, 242);
    expect_int("pixel(6153)", p.pixel, 378);
    expect_int("vblank(6153)", p.vblank, 1);
    expect_int("t1(6153)", p.t1, 1);

    expect_int("frame cycles 0", C7010Pal_FrameCycles(0), 7929);
    expect_int("frame cycles 1", C7010Pal_FrameCycles(1), 7929);
    expect_int("frame cycles 2", C7010Pal_FrameCycles(2), 7930);
    expect_int("frame cycles 3", C7010Pal_FrameCycles(3), 7929);

    for (i = 0; i < 3000; ++i)
        sum += C7010Pal_FrameCycles(i);
    expect_ll("3000-frame total", sum, 3000LL * 7929LL + 1000LL);

    prev = -1;
    for (i = 0; i < 20000; ++i)
    {
        long long phase;
        C7010Pal_Compute((unsigned long)i, &p);
        phase = p.phase_pixels;
        if (phase < 0 || phase >= C7010_PAL_FRAME_PIXELS)
        {
            printf("FAIL phase range at %d: %lld\n", i, phase);
            ++fails;
            break;
        }
        if (prev >= 0 && ((phase - prev + C7010_PAL_FRAME_PIXELS) %
                          C7010_PAL_FRAME_PIXELS) != C7010_PAL_PIXELS_PER_CYCLE)
        {
            printf("FAIL phase continuity at %d: prev=%lld now=%lld\n",
                   i, prev, phase);
            ++fails;
            break;
        }
        prev = phase;
    }
    if (i == 20000)
        printf("PASS phase continuity 20000 cycles\n");

    expect_int("beam line master 0", C7010Pal_BeamLineFromMaster(0), 0);
    expect_int("beam line master 25", C7010Pal_BeamLineFromMaster(25), 1);
    expect_int("beam line master 50", C7010Pal_BeamLineFromMaster(50), 2);
    expect_int("beam line master 6151", C7010Pal_BeamLineFromMaster(6151), 243);
    expect_int("beam line master 7929", C7010Pal_BeamLineFromMaster(7929), 313 - 1);

    expect_int("PAL gate off (non-C7010)", C7010Pal_IsPalMachine(0, 1), 0);
    expect_int("PAL gate off (NTSC)", C7010Pal_IsPalMachine(1, 0), 0);
    expect_int("PAL gate on", C7010Pal_IsPalMachine(1, 1), 1);

    printf("PAL PHASE FAILS=%d\n", fails);
    return fails ? 1 : 0;
}
