
#ifndef WRAP_H
#define WRAP_H 1

#include <unistd.h>
#define INLINE static inline
#define rest(a) usleep(a)
#ifndef __QNX__
#define strupr alleg_upcase
#define strlwr alleg_downcase
#endif
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


BITMAP *create_bitmap(int w,int h);
int destroy_bitmap(BITMAP *buff);
void line(BITMAP  *buff,int x1,int y1,int x2,int y2,unsigned  char  color);
void rect(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color);
void rectfill(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color);
void alleg_upcase(char *p);
void alleg_downcase(char *p);

extern unsigned char key[256*2];

extern void update_joy(void);

#define INLINE static inline

#define EMUWIDTH 340
#define EMUHEIGHT 250

#define TEX_WIDTH 400
#define TEX_HEIGHT 300

#define RGB565(r, g, b)  (((r) << (5+6)) | ((g) << 6) | (b))


#endif

