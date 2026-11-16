# Videopac+ layer refresh and border mixing

The cached EF9340/EF9341 layer previously retained pixels for windows that
became disabled or concealed. Rebuilding starts from black so the display
registers take effect on the whole layer, including the separate service row.
This does not inspect character contents or game identity.

The mixer previously processed the 320-pixel character window and applied
heuristic clearing only above and to its left. The output framebuffer is
wider. Outside the character window, the EF934x black output now passes
through the same VDC-color transparency decoder, independently per pixel.
Opaque VDC colors remain visible. No viewport, palette or artwork is changed.

Technical references:
- Thomson-EFCIS EF9340/EF9341 datasheet, display timing and mode register,
  pages 3 and 14: https://www.retronik.fr/DOCUMENTS/Composants/THOMSON-EFCIS/1981_THOMSON-EFCIS_EF9340-9341.pdf
- MAME's EF9340/EF9341 renderer and G7400 mixer provide an independent
  implementation reference, not a locally reproduced hardware test:
  https://github.com/mamedev/mame/blob/master/src/devices/video/ef9340_1.cpp
  https://github.com/mamedev/mame/blob/master/src/mame/philips/odyssey2.cpp

Focused test (run without NDEBUG; source includes vpp.c to exercise its private
cached layer without CPU, ROM or BIOS dependencies):

```
cc -Ilibretro-common/include -Iallegrowrapper tests/vpp_render_test.c src/vpp_cset.c allegrowrapper/wrapalleg.c -o vpp_render_test
./vpp_render_test
```

The equivalent MSVC C11 build passes locally. The test covers disabled service
row refresh, all 128 combinations of eight decoder inputs and sixteen VDC
palette values across the active window and four borders, and VPP bypass.

Local equivalent-frame runs of Race+Spin+Cryptogram, Helicopter Rescue and
Pickaxe Pete+ remove stale service text and the reported right-side band.
All pixels inside the artwork area below the service row are unchanged in
six compared menu/started images. Helicopter's title was also reproduced
before/after. Its HUD is still being drawn at frame 1199; the complete HUD is
visible at frame 3199. This early frame is not evidence of missing HUD pixels.

This is a local candidate, pending the broader regression phase and user
approval. No hardware-equivalence claim is made. Save-state layout, CPU,
interrupts, sound and C7010 implementation are unchanged.
