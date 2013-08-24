
#ifndef WRAP_H
#define WRAP_H 1

#define INLINE static inline
#define rest(a) usleep(a)
#define strupr upcase
#define strlwr downcase
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



extern unsigned char key[256*2];

#define INLINE static inline


#endif

