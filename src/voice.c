
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
 *   O2 Voice emulation
 */


#include "vmachine.h"
#include "cpu.h"
#include "voice.h"

#include <retro_miscellaneous.h>

#ifdef HAVE_VOICE
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <core_audio_mixer.h>

static core_audio_mixer_sound_t *voices[9][128];
static core_audio_mixer_voice_t *voice = NULL;
static bool voice_finished = true;
#endif
static int voice_bank = 0;
static int voice_addr = 0;
static bool voice_ok  = false;
static int voice_st   = 0;
static unsigned long clk_voice_start = 0;

void init_voice(const char *voice_path)
{
#ifdef HAVE_VOICE
   int n_loaded = 0;
   unsigned i, sam;
   for (i = 0; i < 9; i++)
   {
      for (sam = 0; sam < 128; sam++)
      {
         char file_name[32];
         char file_path[PATH_MAX_LENGTH];
         int64_t file_len    = 0;
         void *file_contents = NULL;
         int            bank = i ? (0xE8 + i - 1) : 0xE4;

         snprintf(file_name, sizeof(file_name), "%02X%02X.WAV", bank, sam + 0x80);
         fill_pathname_join(file_path, voice_path, file_name, sizeof(file_path));

         if (filestream_read_file(file_path, &file_contents, &file_len))
         {
            voices[i][sam] = core_audio_mixer_load_wav(file_contents, file_len,
                  "sinc", RESAMPLER_QUALITY_NORMAL);

            free(file_contents);
            file_contents = NULL;

            if (voices[i][sam])
               n_loaded++;
         }
      }
   }

   voice_bank      = 0;
   voice           = NULL;
   voice_finished  = true;
   voice_addr      = 0;
   voice_st        = 0;
   clk_voice_start = 0;

   voice_ok        = (n_loaded > 0);
#endif
}

#ifdef HAVE_VOICE
void voice_stop_callback(core_audio_mixer_sound_t *sound, unsigned reason)
{
   voice_finished = true;
}
#endif

void update_voice(void)
{
#ifdef HAVE_VOICE
   if (!voice_ok)
      return;

   if (voice_st == 2)
   {
      if (voice_finished)
      {
         if (voice_bank >= 0 && voice_bank <= 8
               && voice_addr >= 0x80 && voice_addr <= 0xff)
         {
            if (voices[voice_bank][voice_addr - 0x80])
            {
               voice = core_audio_mixer_play(voices[voice_bank][voice_addr - 0x80],
                     false, 0, "sinc", RESAMPLER_QUALITY_NORMAL,
                     voice_stop_callback);
               voice_finished = false;
               clk_voice_start = clk_counter;
               voice_st = 1;
            }
            else
               voice_st = 0;
         }
      }
   }
   else if (voice_st == 1)
   {
      if (voice_finished || (clk_counter - clk_voice_start) > 20)
         voice_st = 0;
   }
#endif
}


void trigger_voice(int addr)
{
#ifdef HAVE_VOICE
   if (!voice_ok)
      return;

   if (voice_st)
      update_voice();

   if (voice_st == 0
         && voice_bank >=0 && voice_bank <= 8
         && addr >= 0x80 && addr <= 0xff)
   {
      voice_addr = addr;
      voice_st = 2;
      update_voice();
   }
#endif
}


void set_voice_bank(int bank)
{
#ifdef HAVE_VOICE
   if (!voice_ok)
      return;

   if (bank >= 0 && bank <= 8)
      voice_bank = bank;
#endif
}


int get_voice_status(void)
{
#ifdef HAVE_VOICE
   if (!voice_ok)
      return 0;

   update_voice();
   if (voice_st)
      return 1;
#endif
   return 0;
}


void reset_voice(void)
{
#ifdef HAVE_VOICE
   if (!voice_ok)
      return;

   core_audio_mixer_stop(voice);
   voice      = NULL;
   voice_bank = 0;
   voice_addr = 0;
   voice_st   = 0;
#endif
}


void mute_voice(void)
{
#ifdef HAVE_VOICE
   if (!voice_ok)
      return;

   core_audio_mixer_stop(voice);
   voice = NULL;
#endif
}


void close_voice(void)
{
#ifdef HAVE_VOICE
   unsigned i, sam;

   reset_voice();

   for (i = 0; i < 9; i++)
   {
      for (sam = 0; sam < 128; sam++)
      {
         core_audio_mixer_destroy(voices[i][sam]);
         voices[i][sam] = NULL;
      }
   }
#endif

   voice_ok = false;
}
