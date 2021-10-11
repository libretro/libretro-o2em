
/*
 *   O2EM Free Odyssey2 / Videopac+ Emulator
 *
 *   Created by Daniel Boris <dboris@comcast.net>  (c) 1997,1998
 *
 *   Developed by Andre de la Rocha   <adlroc@users.sourceforge.net>
 *             Arlindo M. de Oliveira <dgtec@users.sourceforge.net>
 *
 *   http://o2em.sourceforge.net
 *
 *
 *
 *   O2 Video Display Controller emulation
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vmachine.h"
#include "config.h"
#include "keyboard.h"
#include "cset.h"
#include "cpu.h"
#include "vpp.h"
#include "vdc.h"

#include "libretro.h"
#include "wrapalleg.h"

#include "audio.h"
#include "voice.h"


#define COL_SP0   0x01
#define COL_SP1   0x02
#define COL_SP2   0x04
#define COL_SP3   0x08
#define COL_VGRID 0x10
#define COL_HGRID 0x20
#define COL_VPP   0x40
#define COL_CHAR  0x80

#define X_START		8
#define Y_START		24

typedef APALETTE PALETTE[256];

static long colortable[2][16]={
	/* O2 palette */
	{0x000000, 0x0e3dd4, 0x00981b, 0x00bbd9, 0xc70008, 0xcc16b3, 0x9d8710, 0xe1dee1,
	 0x5f6e6b, 0x6aa1ff, 0x3df07a, 0x31ffff, 0xff4255, 0xff98ff, 0xd9ad5d, 0xffffff},
	/* VP+ G7400 palette */
	{0x000000, 0x0000b6, 0x00b600, 0x00b6b6, 0xb60000, 0xb600b6, 0xb6b600, 0xb6b6b6,
	 0x494949, 0x4949ff, 0x49ff49, 0x49ffff, 0xff4949, 0xff49ff, 0xffff49, 0xffffff}
	 
};


/* Collision buffer */
static uint8_t *col = NULL;

static PALETTE colors,oldcol;

/* The pointer to the graphics buffer */
static uint8_t *vscreen           = NULL;

static ALLEGRO_BITMAP *bmp                = NULL;
static ALLEGRO_BITMAP *bmpcache           = NULL;
static int cached_lines[MAXLINES];

uint8_t coltab[256];

long clip_low;
long clip_high;

int wsize;

static void create_cmap(void)
{
	int i;

	/* Initialise parts of the colors array */
	for (i = 0; i < 16; i++) {
		/* Use the color values from the color table */
		colors[i+32].r = colors[i].r = (colortable[app_data.vpp?1:0][i] & 0xff0000) >> 16;
		colors[i+32].g = colors[i].g = (colortable[app_data.vpp?1:0][i] & 0x00ff00) >> 8;
		colors[i+32].b = colors[i].b = (colortable[app_data.vpp?1:0][i] & 0x0000ff);
	}

	for (i = 16; i < 32; i++) {
		/* Half-bright colors for the 50% scanlines */
		colors[i+32].r = colors[i].r = colors[i-16].r/2;
		colors[i+32].g = colors[i].g = colors[i-16].g/2;
		colors[i+32].b = colors[i].b = colors[i-16].b/2;
	}

	for (i = 64; i < 256; i++)
      colors[i].r = colors[i].g = colors[i].b = 0;
}

static INLINE void mputvid(unsigned int ad, unsigned int len, uint8_t d, uint8_t c)
{
   if ((ad > (unsigned long)clip_low) && (ad < (unsigned long)clip_high))
   {
      unsigned int i;
      if (((len & 3)==0) && (sizeof(unsigned long) == 4))
      {
         unsigned long dddd = (((unsigned long)d) & 0xff) | ((((unsigned long)d) & 0xff) << 8) | ((((unsigned long)d) & 0xff) << 16) | ((((unsigned long)d) & 0xff) << 24);
         unsigned long cccc = (((unsigned long)c) & 0xff) | ((((unsigned long)c) & 0xff) << 8) | ((((unsigned long)c) & 0xff) << 16) | ((((unsigned long)c) & 0xff) << 24);
         for (i=0; i<len>>2; i++)
         {
            *((unsigned long*)(vscreen+ad)) = dddd;
            cccc |= *((unsigned long*)(col+ad));
            *((unsigned long*)(col+ad)) = cccc;
            coltab[c] |= ((cccc | (cccc >> 8) | (cccc >> 16) | (cccc >> 24)) & 0xff);
            ad += 4;
         }
      } else {
         for (i=0; i<len; i++) {
            vscreen[ad]=d;
            col[ad] |= c;
            coltab[c] |= col[ad++];
         }
      }
   }
}

static void draw_char(uint8_t ypos,uint8_t xpos,uint8_t chr,uint8_t col)
{
	int j;
	uint8_t cl,d1;
	int b,n;
	int y=(ypos & 0xFE); 
	unsigned int pnt = y * BMPW + ((xpos-8) * 2)+20;

	ypos = ypos >> 1;
	n    = 8 - (ypos % 8) - (chr % 8);

	if (n < 3)
      n = n + 7;
	
	if ((pnt+BMPW*2*n >= (unsigned long)clip_low) && (pnt <= (unsigned long)clip_high))
   {
      int c=(int)chr + ypos;
      if (col & 0x01)
         c+=256;
      if (c > 511)
         c=c-512;

      cl = ((col & 0x0E) >> 1);
      cl = ((cl&2) | ((cl&1)<<2) | ((cl&4)>>2)) + 8;

      if ((y>0) && (y<232) && (xpos<157))
      {
         for (j=0; j<n; j++)
         {
            d1 = cset[c+j];
            for (b=0; b<8; b++)
            {
               if (d1 & 0x80)
               {
                  if ((xpos-8+b < 160) && (y+j < 240))
                  {
                     mputvid(pnt,2,cl,COL_CHAR);
                     mputvid(pnt+BMPW,2,cl,COL_CHAR);
                  }
               }
               pnt+=2;
               d1 = d1 << 1;
            }
            pnt += BMPW*2-16;
         }
      }
   }
}

/* This quad drawing routine can display the quad cut off effect used in KTAA.
 * It needs more testing with other games, especially the clipping.
 * This code is quite slow and needs a rewrite by somebody with more experience
 * than I (sgust) have */

static void draw_quad(uint8_t ypos, uint8_t xpos, uint8_t cp0l, uint8_t cp0h, uint8_t cp1l, uint8_t cp1h, uint8_t cp2l, uint8_t cp2h, uint8_t cp3l, uint8_t cp3h)
{
	/* char set pointers */
	int chp[4];
	/* colors */
	uint8_t col[4];
	/* pointer into screen bitmap */
	unsigned int pnt;
	/* offset into current line */
	unsigned int off;
	/* loop variables */
	int i, j, lines;

	/* get screen bitmap position of quad */
	pnt = (ypos & 0xfe) * BMPW + ((xpos - 8) * 2) + 20;
	/* abort drawing if completely below the bottom clip */
	if (pnt > (unsigned long) clip_high) return;
	/* extract and convert char-set offsets */
	chp[0] = cp0l | ((cp0h & 1) << 8);
	chp[1] = cp1l | ((cp1h & 1) << 8);
	chp[2] = cp2l | ((cp2h & 1) << 8);
	chp[3] = cp3l | ((cp3h & 1) << 8);
	for(i = 0; i < 4; i++) chp[i] = (chp[i] + (ypos >> 1)) & 0x1ff;
	lines = 8 - (chp[3]+1) % 8;
	/* abort drawing if completely over the top clip */
	if (pnt+BMPW*2*lines < (unsigned long) clip_low) return;
	/* extract and convert color information */
	col[0] = (cp0h & 0xe) >> 1;
	col[1] = (cp1h & 0xe) >> 1;
	col[2] = (cp2h & 0xe) >> 1;
	col[3] = (cp3h & 0xe) >> 1;
	for(i = 0; i < 4; i++) col[i] = ((col[i] & 2) | ((col[i] & 1) << 2) | ((col[i] & 4) >> 2)) + 8;
	/* now draw the quad line by line controlled by the last quad */
	while(lines-- > 0)
   {
		off = 0;
		/* draw all 4 sub-quads */
		for(i = 0; i < 4; i++) {
			/* draw sub-quad pixel by pixel, but stay in same line */
			for(j = 0; j < 8; j++) {
				if((cset[chp[i]] & (1 << (7-j))) && (off < BMPW)) {
					mputvid(pnt+off, 2, col[i], COL_CHAR);
					mputvid(pnt+off+BMPW, 2, col[i], COL_CHAR);
				}
				/* next pixel */
				off += 2;
			}
			/* space between sub-quads */
			off += 16;
		}
		/* advance char-set pointers */
		for(i = 0; i < 4; i++) chp[i] = (chp[i]+1) & 0x1ff;
		/* advance screen bitmap pointer */
		pnt += BMPW*2;
	}
}

static void draw_grid(void)
{
   unsigned int pnt, pn1;
   uint8_t mask,d;
   int j,i,x,w;
   uint8_t color;

   if (VDCwrite[0xA0] & 0x40) {
      for(j=0; j<9; j++) {

         pnt = (((j*24)+24) * BMPW);
         for (i=0; i<9; i++) {
            pn1 = pnt + (i * 32) + 20;
            color = ColorVector[j*24+24];
            mputvid(pn1, 4, (color & 0x07) | ((color & 0x40) >> 3) | ((color & 0x80) ? 0 : 8), COL_HGRID);
            color = ColorVector[j*24+25];
            mputvid(pn1+BMPW, 4, (color & 0x07) | ((color & 0x40) >> 3) | ((color & 0x80) ? 0 : 8), COL_HGRID);
            color = ColorVector[j*24+26];
            mputvid(pn1+BMPW*2, 4, (color & 0x07) | ((color & 0x40) >> 3) | ((color & 0x80) ? 0 : 8), COL_HGRID);
         }
      }
   }

   mask=0x01;
   for(j=0; j<9; j++) {
      pnt = (((j*24)+24) * BMPW);
      for (i=0; i<9; i++) {
         pn1 = pnt + (i * 32) + 20;
         if ((pn1+BMPW*3 >= (unsigned long)clip_low) && (pn1 <= (unsigned long)clip_high)) {
            d=VDCwrite[0xC0 + i];
            if (j == 8) {
               d=VDCwrite[0xD0+i];
               mask=1;
            }
            if (d & mask)	{
               color = ColorVector[j*24+24];
               mputvid(pn1, 36, (color & 0x07) | ((color & 0x40) >> 3) | ((color & 0x80) ? 0 : 8), COL_HGRID);
               color = ColorVector[j*24+25];
               mputvid(pn1+BMPW, 36, (color & 0x07) | ((color & 0x40) >> 3) | ((color & 0x80) ? 0 : 8), COL_HGRID);
               color = ColorVector[j*24+26];
               mputvid(pn1+BMPW*2, 36, (color & 0x07) | ((color & 0x40) >> 3) | ((color & 0x80) ? 0 : 8), COL_HGRID);
            }
         }
      }
      mask = mask << 1;
   }

   mask=0x01;
   w=4;
   if (VDCwrite[0xA0] & 0x80) w=32;
   for(j=0; j<10; j++) {

      pnt=(j*32);
      mask=0x01;
      d=VDCwrite[0xE0+j];
      for (x=0; x<8; x++) {
         pn1 = pnt + (((x*24)+24) * BMPW) + 20;
         if (d & mask) {
            for(i=0; i<24; i++) {
               if ((pn1 >= (unsigned long)clip_low) && (pn1 <= (unsigned long)clip_high)) {
                  color = ColorVector[x*24+24+i];
                  mputvid(pn1, w, (color & 0x07) | ((color & 0x40) >> 3) | (color & 0x80 ? 0 : 8), COL_VGRID);
               }
               pn1+=BMPW;

            }
         }
         mask = mask << 1;
      }
   }
}

void draw_display(void)
{
   int i,j,x,sm,t;
   uint8_t y,b,d1,cl,c;
   unsigned int pnt,pnt2;

	for (i=clip_low/BMPW; i<clip_high/BMPW; i++)
      memset(vscreen+i*BMPW, ((ColorVector[i] & 0x38) >> 3) | (ColorVector[i] & 0x80 ? 0 : 8), BMPW);

	if (VDCwrite[0xA0] & 0x08)
      draw_grid();

	if (useforen && (!(VDCwrite[0xA0] & 0x20)))
      return;
	
	for(i=0x10; i<0x40; i+=4)
      draw_char(VDCwrite[i],VDCwrite[i+1],VDCwrite[i+2],VDCwrite[i+3]);

	/* draw quads, position mapping happens in ext_write (vmachine.c)*/
	for(i = 0x40; i < 0x80; i+= 0x10)
	draw_quad(VDCwrite[i], VDCwrite[i+1], VDCwrite[i+2], VDCwrite[i+3],
	VDCwrite[i+6], VDCwrite[i+7],
	VDCwrite[i+10], VDCwrite[i+11],
	VDCwrite[i+14], VDCwrite[i+15]);
	
    c=8;
	for (i=12; i>=0; i -=4) {
		pnt2 = 0x80 + (i * 2);
		y = VDCwrite[i];
		x = VDCwrite[i+1]-8;
		t = VDCwrite[i+2];
		cl = ((t & 0x38) >> 3);
		cl = ((cl&2) | ((cl&1)<<2) | ((cl&4)>>2)) + 8;
		/*174*/
        if ((x<164) && (y>0) && (y<232)) {
			pnt = y * BMPW + (x * 2) + 20 + sproff;
			if (t & 4) {
				if ((pnt+BMPW*32 >= (unsigned long)clip_low) && (pnt <= (unsigned long)clip_high)) {
					for (j=0; j<8; j++) {
						sm = (((j%2==0) && (((t>>1) & 1) != (t & 1))) || ((j%2==1) && (t & 1))) ? 1 : 0;
						d1 = VDCwrite[pnt2++];
						for (b=0; b<8; b++) {
							if (d1 & 0x01) {
								if ((x+b+sm < 159) && (y+j < 247)) {
									mputvid(sm+pnt,4,cl,c);
									mputvid(sm+pnt+BMPW,4,cl,c);
									mputvid(sm+pnt+2*BMPW,4,cl,c);
									mputvid(sm+pnt+3*BMPW,4,cl,c);
								}
							}
							pnt += 4;
							d1 = d1 >> 1;
						}
						pnt += BMPW*4-32;
					}
				}
			} else {
				if ((pnt+BMPW*16 >= (unsigned long)clip_low) && (pnt <= (unsigned long)clip_high)) {
					for (j=0; j<8; j++) {
						sm = (((j%2==0) && (((t>>1) & 1) != (t & 1))) || ((j%2==1) && (t & 1))) ? 1 : 0;
						d1 = VDCwrite[pnt2++];
						for (b=0; b<8; b++) {
							if (d1 & 0x01) {
								if ((x+b+sm<160) && (y+j<249)) {
									mputvid(sm+pnt,2,cl,c);
									mputvid(sm+pnt+BMPW,2,cl,c);
								}
							}
							pnt += 2;
							d1 = d1 >> 1;
						}
						pnt += BMPW*2-16;
					}
				}
			}
		}
		c = c >> 1;
	}
}

void draw_region(void){
    int i;

    if (regionoff == 0xffff)
		i = (master_clk/(LINECNT-1)-5);
	else
		i = (master_clk/22+regionoff); 
	    i = (snapline(i, VDCwrite[0xA0], 0));

	
    if (app_data.crc == 0xA7344D1F)
                      {i = (master_clk/22+regionoff)+6; 
                       i = (snapline(i, VDCwrite[0xA0], 0)+6);
                      }/*Atlantis*/      

    if (app_data.crc == 0xD0BC4EE6)
                      {i = (master_clk/24+regionoff)-6; 
                       i = (snapline(i, VDCwrite[0xA0], 0)+7);
                      }/*Frogger*/
                      
    if (app_data.crc == 0x26517E77)
                      {i = (master_clk/22+regionoff); 
                       i = (snapline(i, VDCwrite[0xA0], 0)-5);
                      }/*Comando Noturno*/

    if (app_data.crc == 0xA57E1724)
                      {i = (master_clk/(LINECNT-1)-5); 
                       i = (snapline(i, VDCwrite[0xA0], 0)-3);
                      }/*Catch the ball*/

    if (i<0) i=0;
 	clip_low = last_line * (long)BMPW;
	clip_high = i * (long)BMPW;
	if (clip_high > BMPW*BMPH) clip_high = BMPW*BMPH;
	if (clip_low < 0) clip_low=0; 
	if (clip_low < clip_high)
      draw_display();
	last_line=i;   

}


void grmode(void)
{
   wsize = 1;
   clearscr();
}


void set_textmode(void)
{
   if (new_int)
      Set_Old_Int9();
}


void clearscr(void)
{
}



extern uint16_t mbmp[TEX_WIDTH  * TEX_HEIGHT];

void retro_blit(void)
{
	int i,j;
	unsigned  char ind;
	uint16_t *outp = &mbmp[0];
	uint8_t  *inp  = &bmp->line[0];

	for(i=0;i<250;i++)
   {
		for(j=0;j<340;j++)
      {
			ind=inp[i*340 + j];
#if defined(SUPPORT_ABGR1555)
      // Hack for PS2 that expects ABGR1555 encoded pixels
			(*outp++) = ABGR1555(colors[ind].r, colors[ind].g, colors[ind].b);
#else
			(*outp++) = RGB565(colors[ind].r, colors[ind].g, colors[ind].b);
#endif
		}
		outp+=	(TEX_WIDTH-340);
	}
}

void retro_destroybmp(void)
{
   destroy_bitmap(bmp);
   bmp = NULL;
   destroy_bitmap(bmpcache);
   bmpcache = NULL;
}

void finish_display(void)
{
	int x,y,sn;
	static int cache_counter=0;

	vpp_finish_bmp(vscreen, 9, 5, BMPW-9, BMPH-5, bmp->w, bmp->h);

	for (y=0; y<bmp->h; y++)
   {
		cached_lines[y] = !memcmp(&bmpcache->line[y*bmpcache->w], &bmp->line[y*bmp->w], bmp->w);
		if (!cached_lines[y])
         memcpy(&bmpcache->line[y*bmpcache->w], &bmp->line[y*bmp->w], bmp->w);
	}

	for (y=0; y<10; y++) cached_lines[(y+cache_counter) % bmp->h] = 0;
	cache_counter = (cache_counter+10) % bmp->h;

	sn = ((wsize>1) && (app_data.scanlines)) ? 1 : 0;

	if (sn)
   {
		for (y=0; y<WNDH; y++)
      {
         if (!cached_lines[y+2])
         {
            for (x=0; x<bmp->w; x++)
               bmp->line[(y+2)*bmp->w +x] += 16;
            memcpy(&bmp->line[(y+2)*bmp->w], &bmpcache->line[(y+2)*bmpcache->w], bmp->w);
         }
      }
	}

	retro_blit();
}

void clear_collision(void)
{
	load_colplus(col);
	coltab[0x01]=coltab[0x02]=0;
	coltab[0x04]=coltab[0x08]=0;
	coltab[0x10]=coltab[0x20]=0;
	coltab[0x40]=coltab[0x80]=0;
}


void close_display(void) {
   if (col)
      free(col);
   col = NULL;
}


void window_close_hook(void){
	key_debug=0;
	key_done=1;
}

void display_bg(void)
{
	rectfill(bmp,20,72,311,172,9+32);
	line(bmp,20,72,311,72,15+32);
	line(bmp,20,72,20,172,15+32);
	line(bmp,21,172,311,172,1+32);
	line(bmp,311,172,311,72,1+32);
}

void init_display(void)
{
   create_cmap();
   bmp = create_bitmap(BMPW,BMPH);
   bmpcache = create_bitmap(BMPW,BMPH);

   if ((!bmp) || (!bmpcache)) {
      exit(EXIT_FAILURE);
   }
   vscreen = (uint8_t *) &bmp->line[0];

   col = (uint8_t *)malloc(BMPW*BMPH);
   if (!col)
   {
      free(vscreen);
      exit(EXIT_FAILURE);
   }
   memset(col,0,BMPW*BMPH);

   if (!app_data.debug)
   {
      grmode();
      init_keyboard();
   }
}
