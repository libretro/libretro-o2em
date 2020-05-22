/* Utility functions to write UI elements on the screen */

#ifndef __VKEYB_UI_H
#define __VKEYB_UI_H

#include <stdint.h>

// Draw an image at the given position
extern void draw_bmp(int x, int y, const uint16_t *img, int img_width, int img_height);
// Draw a colored box at the given position
extern void draw_box(int x, int y, int width, int height, int thickness, uint16_t color);

#endif /* __VKEYB_UI_H */
