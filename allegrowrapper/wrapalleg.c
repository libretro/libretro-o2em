
#include "wrapalleg.h"

#include <stdio.h>
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

BITMAP *create_bitmap(int w,int h)
{
   BITMAP *buff = malloc(sizeof (BITMAP));

   if (!buff)
      return NULL;

   buff->line   = malloc(1*w*h);

   buff->w     = w;
   buff->h     = h;
   buff->pitch = w;
   buff->depth = 1;  

   return buff;
}

int destroy_bitmap(BITMAP *buff)
{
   if (buff)
   {
      if (buff->line)
         free(buff->line);

      free(buff);
   }

   return 0;
}


void rect(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color)       { }
void rectfill(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color)   { }
void DrawHline(BITMAP  *buff,int x,int y,int dx,int dy,unsigned  char  color) { }
void DrawVline(BITMAP *buff,int x,int y,int dx,int dy,unsigned  char  color)  { }
void line(BITMAP  *buff,int x1,int y1,int x2,int y2,unsigned  char  color)    { }
