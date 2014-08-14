
#include "wrapalleg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define abs(a) miabs(a)

int miabs(int a){
	if((a)<0)a=-a;
	return a;
}

void upcase(char *p)
{
	while(*p != '\0')
	{
		if(*p >= 97 && *p <= 122)
			*p -= 32;
		++p;
	}
}
void downcase(char *p)
{
	while(*p != '\0')
	{
		if(*p >= 97-32 && *p <= 122-32)
			*p += 32;
		++p;
	}
}

void clear(BITMAP *buff){

	memset(&buff->line,0,buff->w*buff->h);
}

BITMAP *create_bitmap(int w,int h){

	 BITMAP  *buff;

	 buff = malloc(sizeof (BITMAP));
	 buff->line=malloc(1*w*h);

	 buff->w=w;
	 buff->h=h;
	 buff->pitch=w;
	 buff->depth=1;  

	return buff;
}

int destroy_bitmap(BITMAP *buff){

     	if (buff->line) {
        	free(buff->line);
     	}
     	free(buff);

	return 0;
}


void rect(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color){
	
//	int i,j,idx;
//				
//	int dx= abs(x2-x);
//	int dy= abs(y2-y);
//	int V_WIDTH = buff->w;
//	unsigned char *buffer=buff->line[0];
//
//	for(i=x;i<x+dx;i++){
//		idx=i+y*V_WIDTH;
//		buffer[idx]=color;
//		idx=i+(y+dy)*V_WIDTH;
//		buffer[idx]=color;
//	}
//
//	for(j=y;j<y+dy;j++){
//			
//		idx=x+j*V_WIDTH;
//		buffer[idx]=color;	
//		idx=(x+dx)+j*V_WIDTH;
//		buffer[idx]=color;	
//	}
	
}
void rectfill(BITMAP  *buff,int x,int y,int x2,int y2,unsigned  char color){
	
//	int i,j,idx;
//		
//	int dx= abs(x2-x);
//	int dy= abs(y2-y);
//	int V_WIDTH = buff->w;
//	unsigned char *buffer=buff->line[0];
//
//	for(i=x;i<x+dx;i++){
//		for(j=y;j<y+dy;j++){
//			
//			idx=i+j*V_WIDTH;
//			buffer[idx]=color;	
//		}
//	}
	
}

void DrawHline(BITMAP  *buff,int x,int y,int dx,int dy,unsigned  char  color){
	
//	int i,j,idx;
//	int V_WIDTH = buff->w;
//	unsigned char *buffer=buff->line[0];
//
//	for(i=x;i<x+dx;i++){
//		idx=i+y*V_WIDTH;
//		buffer[idx]=color;		
//	}
}

void DrawVline(BITMAP *buff,int x,int y,int dx,int dy,unsigned  char  color){
	
//	int i,j,idx;
//	int V_WIDTH = buff->w;
//	unsigned char *buffer=buff->line[0];
//
//	for(j=y;j<y+dy;j++){			
//		idx=x+j*V_WIDTH;
//		buffer[idx]=color;		
//	}	
}

void line(BITMAP  *buff,int x1,int y1,int x2,int y2,unsigned  char  color){
		
//	int pixx, pixy;
// 	int x, y;
// 	int dx, dy; 	
// 	int sx, sy;
// 	int swaptmp;
// 	int idx;
//
//	int V_WIDTH = buff->w;
//	unsigned char *buffer=buff->line[0];
//
//	dx = x2 - x1;
//	dy = y2 - y1;
//	sx = (dx >= 0) ? 1 : -1;
//	sy = (dy >= 0) ? 1 : -1;
//
//	if (dx==0) {
// 		if (dy>0) {
// 			DrawVline(buff, x1, y1,0, dy, color);
//			return;
//
// 		} else if (dy<0) {
// 			DrawVline(buff, x1, y2,0, -dy, color);
//			return;
//
// 		} else {
//			idx=x1+y1*V_WIDTH;
// 			buffer[idx]=color;
//			return ;
// 		}
// 	}
// 	if (dy == 0) {
// 		if (dx>0) {
// 			DrawHline(buff, x1, y1, dx, 0, color);
//			return;
//
// 		} else if (dx<0) {
// 			DrawHline(buff, x2, y1, -dx,0, color);
//			return;
// 		}
// 	}
//
//	dx = sx * dx + 1;
// 	dy = sy * dy + 1;
//	
//	pixx = 1;
// 	pixy = V_WIDTH;
//
// 	pixx *= sx;
// 	pixy *= sy;
//
// 	if (dx < dy) {
//	 	swaptmp = dx;
//	 	dx = dy;
//	 	dy = swaptmp;
//	 	swaptmp = pixx;
//	 	pixx = pixy;
//	 	pixy = swaptmp;
// 	}
//
//	x = 0;
// 	y = 0;
//
//	idx=x1+y1*V_WIDTH;
//
//	for (; x < dx; x++, idx +=pixx) {
//		buffer[idx]=color;
// 		y += dy;
// 		if (y >= dx) {
// 			y -= dx;
// 			idx += pixy;
// 		}
//	}

}
