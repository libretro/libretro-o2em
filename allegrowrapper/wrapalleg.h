#ifndef WRAP_H
#define WRAP_H

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <retro_inline.h>

#define keypressed() 0
#define poll_keyboard()
#define yield_timeslice()

typedef struct
{  
   unsigned char *line;   
   int w;
   int h;
   int pitch;
   int depth;   
}BITMAP;

typedef struct
{
   unsigned char r;
   unsigned char g;
   unsigned char b;
}APALETTE;


extern BITMAP *create_bitmap(int w,int h);
extern int destroy_bitmap(BITMAP *buff);
extern void line(BITMAP  *buff,int x1,int y1,int x2,int y2,unsigned  char  color);
extern void rect(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color);
extern void rectfill(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color);
extern void alleg_upcase(char *p);
extern void alleg_downcase(char *p);

extern unsigned char key[256*2];

extern void update_joy(void);

#define EMUWIDTH 340
#define EMUHEIGHT 250

#define TEX_WIDTH 400
#define TEX_HEIGHT 300

#define RGB565(r, g, b)   ((((r) << 8) &  0xf800) | (((g) << 3) & 0x7e0) | (((b) >> 3) & 0x1f))
#define ABGR1555(r, g, b) ((((b) << 7) &  0x7C00) | (((g) << 2) & 0x3e0) | (((r) >> 3) & 0x1f))

#endif
