/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (core_audio_mixer.h).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LIBRETRO_CORE_AUDIO_MIXER__H
#define __LIBRETRO_CORE_AUDIO_MIXER__H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boolean.h>
#include <retro_common_api.h>

#include <audio/audio_resampler.h>

RETRO_BEGIN_DECLS

enum core_audio_mixer_type
{
   CORE_AUDIO_MIXER_TYPE_NONE = 0,
   CORE_AUDIO_MIXER_TYPE_WAV,
   CORE_AUDIO_MIXER_TYPE_OGG,
   CORE_AUDIO_MIXER_TYPE_MOD,
   CORE_AUDIO_MIXER_TYPE_FLAC,
   CORE_AUDIO_MIXER_TYPE_MP3
};

typedef struct core_audio_mixer_sound core_audio_mixer_sound_t;
typedef struct core_audio_mixer_voice core_audio_mixer_voice_t;

typedef void (*core_audio_mixer_stop_cb_t)(core_audio_mixer_sound_t* sound, unsigned reason);

/* Reasons passed to the stop callback. */
#define CORE_AUDIO_MIXER_SOUND_FINISHED 0
#define CORE_AUDIO_MIXER_SOUND_STOPPED  1
#define CORE_AUDIO_MIXER_SOUND_REPEATED 2

void core_audio_mixer_init(unsigned rate);

void core_audio_mixer_done(void);

core_audio_mixer_sound_t* core_audio_mixer_load_wav(void *buffer, int32_t size,
      const char *resampler_ident, enum resampler_quality quality);
core_audio_mixer_sound_t* core_audio_mixer_load_ogg(void *buffer, int32_t size);
core_audio_mixer_sound_t* core_audio_mixer_load_mod(void *buffer, int32_t size);
core_audio_mixer_sound_t* core_audio_mixer_load_flac(void *buffer, int32_t size);
core_audio_mixer_sound_t* core_audio_mixer_load_mp3(void *buffer, int32_t size);

void core_audio_mixer_destroy(core_audio_mixer_sound_t* sound);

core_audio_mixer_voice_t* core_audio_mixer_play(core_audio_mixer_sound_t* sound,
      bool repeat, float volume,
      const char *resampler_ident,
      enum resampler_quality quality,
      core_audio_mixer_stop_cb_t stop_cb);

void core_audio_mixer_stop(core_audio_mixer_voice_t* voice);

float core_audio_mixer_voice_get_volume(core_audio_mixer_voice_t *voice);

void core_audio_mixer_voice_set_volume(core_audio_mixer_voice_t *voice, float val);

void core_audio_mixer_mix(float* buffer, size_t num_frames, float volume_override, bool override);

RETRO_END_DECLS

#endif
