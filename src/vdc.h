#ifndef __VDC_H
#define __VDC_H

#include <stdint.h>

#define BMPW 340
#define BMPH 250
#define WNDW 320
#define WNDH 240
/*********/
#define BOX_W     MIN(512, SCREEN_W-16)
#define BOX_H     MIN(256, (SCREEN_H-64)&0xFFF0)

#define BOX_L     ((SCREEN_W - BOX_W) / 2)
#define BOX_R     ((SCREEN_W + BOX_W) / 2)
#define BOX_T     ((SCREEN_H - BOX_H) / 2)
#define BOX_B     ((SCREEN_H + BOX_H) / 2)

/*********/
extern uint8_t coltab[];
extern long clip_low;
extern long clip_high;

void init_display(void);
void draw_display(void);
void set_textmode(void);
void draw_region(void);
void finish_display();
void close_display(void);
void grmode(void);
void display_bg(void);
void clear_collision(void);
void clearscr(void);

#endif


