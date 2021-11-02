
#include "wrapalleg.h"

#include <stdlib.h>
#include <string.h>

void alleg_downcase(char *p)
{
   while(*p != '\0')
   {
      if(*p >= 97-32 && *p <= 122-32)
         *p += 32;
      ++p;
   }
}

ALLEGRO_BITMAP *create_bitmap(int w,int h)
{
   ALLEGRO_BITMAP *buff = (ALLEGRO_BITMAP*)
      malloc(sizeof (ALLEGRO_BITMAP));

   if (!buff)
      return NULL;

   buff->line   = malloc(1*w*h);

   buff->w      = w;
   buff->h      = h;
   buff->pitch  = w;
   buff->depth  = 1;  

   return buff;
}

int destroy_bitmap(ALLEGRO_BITMAP *buff)
{
   if (buff)
   {
      if (buff->line)
         free(buff->line);
      buff->line = NULL;
      free(buff);
   }

   return 0;
}


void rect(ALLEGRO_BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color)       { }
void rectfill(ALLEGRO_BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color)   { }
void DrawHline(ALLEGRO_BITMAP  *buff,int x,int y,int dx,int dy,unsigned  char  color) { }
void DrawVline(ALLEGRO_BITMAP *buff,int x,int y,int dx,int dy,unsigned  char  color)  { }
void line(ALLEGRO_BITMAP  *buff,int x1,int y1,int x2,int y2,unsigned  char  color)    { }
