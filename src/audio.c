
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
 *   O2 audio emulation
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "config.h"
#include "vmachine.h"
#include "audio.h"

/* i8244 sound shift clock: one shift every 4 scanlines when control
 * bit 0x20 is set, every 16 when clear (MAME i8244). Expressed in
 * 16.16 fixed-point sample intervals at 1056 samples per frame:
 *   NTSC (15734.264 Hz line rate): 16.1075 / 64.4301 samples
 *   PAL  (15625 Hz line rate):     13.5168 / 54.0672 samples
 * The previous integer periods 11/44 ran the shift clock 46%
 * (NTSC) / 23% (PAL) too fast, raising every tone by 6.6 / 3.6
 * semitones against hardware. */
#define PERIOD_FAST_NTSC 1055623UL
#define PERIOD_SLOW_NTSC 4222490UL
#define PERIOD_FAST_PAL  885837UL
#define PERIOD_SLOW_PAL  3543348UL

#define AUD_CTRL  0xAA
#define AUD_D0	  0xA7
#define AUD_D1	  0xA8
#define AUD_D2	  0xA9

extern uint8_t soundBuffer[SOUND_BUFFER_LEN];

int sound_IRQ;
uint32_t snd_period_acc;

int snd_shift_count;

static double flt_a=0.0, flt_b=0.0;
static unsigned char flt_prv = 0;


static void filter(unsigned char *buffer, unsigned long len)
{
	static unsigned char buf[SOUND_BUFFER_LEN];
	unsigned long i;
	if (len>SOUND_BUFFER_LEN)
      return;

	memcpy(buf,buffer,len);	

	for (i=0; i<len; i++)
   {
		int t = (i==0)?(buf[0]-flt_prv):(buf[i]-buf[i-1]);
		if (t) flt_b = (double)t;
		flt_a += flt_b/4.0 - flt_a/80.0;
		flt_b -= flt_b/4.0;
		if ((flt_a>255.0)||(flt_a<-255.0))
         flt_a=0.0;
		buffer[i] = (unsigned char)((flt_a+255.0)/2.0);
	}
	flt_prv = buf[len-1];
}

void audio_process(unsigned char *buffer)
{
   unsigned long aud_data = (VDCwrite[AUD_D2] | 
         (VDCwrite[AUD_D1] << 8) | (VDCwrite[AUD_D0] << 16));

   int intena = VDCwrite[0xA0] & 0x04;
   int pnt    = 0;
   uint32_t period_fast = (fps == FPS_NTSC) ? PERIOD_FAST_NTSC : PERIOD_FAST_PAL;
   uint32_t period_slow = (fps == FPS_NTSC) ? PERIOD_SLOW_NTSC : PERIOD_SLOW_PAL;

   while (pnt < SOUND_BUFFER_LEN)
   {
      int pos     = (tweakedaudio) ? (pnt/3) : (MAXLINES-1);
      int volume  = AudioVector[pos] & 0x0F;
      int enabled = AudioVector[pos] & 0x80;
      uint32_t period = (AudioVector[pos] & 0x20) ? period_fast : period_slow;

      buffer[pnt++] = (enabled) ? (aud_data & 0x01) * (0x10 * volume) : 0;
      snd_period_acc += 0x10000;

      if (snd_period_acc >= period)
      {
         /* i8244 24-bit sound shift register, verified against MAME
          * i8244.cpp: the output bit recirculates into bit 23
          * unconditionally (control register bit 0x40 is not connected
          * on real hardware); in noise mode the tap is on bits 0 and 5
          * (pre-shift) and the feedback is also fed to bit 15. */
         unsigned long feedback = aud_data & 0x01;

         snd_period_acc -= period;
         aud_data >>= 1;

         if (AudioVector[pos] & 0x10)
         {
            feedback ^= (aud_data >> 4) & 0x01; /* pre-shift bit 5 */
            aud_data  = (aud_data & ~0x8000UL) | (feedback << 15);
         }

         aud_data |= feedback << 23;

         /* the hardware raises the sound interrupt every 24 shift
          * register clocks when enabled via control register bit 2 */
         if (++snd_shift_count >= 24)
         {
            snd_shift_count = 0;
            if (intena && (!sound_IRQ))
            {
               sound_IRQ = 1;
               ext_IRQ();
            }
         }
      }
   }

   /* shift register state persists in the VDC registers across frames */
   VDCwrite[AUD_D2] = aud_data & 0xFF;
   VDCwrite[AUD_D1] = (aud_data >> 8) & 0xFF;
   VDCwrite[AUD_D0] = (aud_data >> 16) & 0xFF;

   if (app_data.filter)
      filter(buffer, SOUND_BUFFER_LEN);
}



void update_audio(void)
{
   audio_process(soundBuffer);
}


void init_audio(void)
{
   sound_IRQ=0;
   snd_shift_count=0;
   snd_period_acc=0;
   if ((app_data.sound_en) || (app_data.voice))
      init_sound_stream();
}


void init_sound_stream(void)
{
   if (app_data.sound_en)
   {
#if 0
      int vol;
      if (app_data.filter)
         vol = (255*app_data.svolume)/100;
      else
         vol = (255*app_data.svolume)/200;
#endif
      flt_a   = flt_b = 0.0;
      flt_prv = 0;
   }
}


void mute_audio(void)
{
}

void close_audio(void)
{
	app_data.sound_en=0;
}


