/* Lifecycle and reentrancy regression test.
 *
 * Modes:
 *   cycles N   N full init/load/run/unload/deinit cycles in one
 *              process, hashing 90 frames of video and audio output
 *              and the serialized machine state after each cycle.
 *              Every cycle must match cycle 0 exactly: any static
 *              that is not re-initialized shows up here. This is the
 *              usage pattern of statically linked console ports.
 *   reload N   one init, N load/unload/load cycles. Primarily a leak
 *              detector: build the core and this test with
 *              -fsanitize=address and run with
 *              ASAN_OPTIONS=detect_leaks=1 so LeakSanitizer fails the
 *              process on per-reload leaks.
 *   soak N     one lifecycle, N frames, verifying the libretro A/V
 *              contract: exactly one audio batch of a constant number
 *              of frames per retro_run.
 *
 * Usage: lifecycle <path/to/o2em_libretro.so> <mode> <N>
 * Exit code 0 on success.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "libretro.h"
#include "testrom.h"

static char workdir[256];

static bool env_cb(unsigned cmd, void *data)
{
   if (cmd == RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY)
   {
      *(const char **)data = workdir;
      return true;
   }
   if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
      return true;
   return false;
}

static unsigned long video_hash;
static void vid_cb(const void *d, unsigned w, unsigned h, size_t pitch)
{
   const unsigned char *q = (const unsigned char *)d;
   size_t i;
   (void)w;
   if (!d)
      return;
   for (i = 0; i < h * pitch; i++)
      video_hash = (video_hash ^ q[i]) * 16777619UL;
}

static unsigned long audio_hash;
static long audio_count;
static long audio_batches;
static void aud_cb(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audb_cb(const int16_t *d, size_t frames)
{
   size_t i;
   for (i = 0; i < frames * 2; i++)
      audio_hash = (audio_hash ^ (unsigned short)d[i]) * 16777619UL;
   audio_count += (long)frames;
   audio_batches++;
   return frames;
}
static void inp_poll(void) { }
static int16_t inp_state(unsigned a, unsigned b, unsigned c, unsigned d)
{ (void)a; (void)b; (void)c; (void)d; return 0; }

int main(int argc, char **argv)
{
   const char *mode;
   int n, cyc, i, rc = 0;
   static unsigned char cart[TESTROM_CART_SIZE];
   struct retro_game_info gi;
   char tmpl[] = "/tmp/o2em_lifecycle_XXXXXX";
   void *h;

   if (argc < 4)
   {
      fprintf(stderr, "usage: %s <o2em_libretro.so> cycles|reload|soak <N>\n",
              argv[0]);
      return 2;
   }
   mode = argv[2];
   n    = atoi(argv[3]);

   if (!mkdtemp(tmpl))
      return 2;
   snprintf(workdir, sizeof(workdir), "%s", tmpl);
   testrom_write_images(workdir);
   {
      char p[512];
      FILE *f;
      size_t got;
      snprintf(p, sizeof(p), "%s/cart.bin", workdir);
      f = fopen(p, "rb");
      got = fread(cart, 1, sizeof(cart), f);
      fclose(f);
      if (got != sizeof(cart))
         return 2;
   }

   h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!h)
   {
      fprintf(stderr, "dlopen: %s\n", dlerror());
      return 2;
   }
#define SYM(x) __typeof__(x) *x##_ = dlsym(h, #x)
   SYM(retro_set_environment);       SYM(retro_set_video_refresh);
   SYM(retro_set_audio_sample);      SYM(retro_set_audio_sample_batch);
   SYM(retro_set_input_poll);        SYM(retro_set_input_state);
   SYM(retro_init);                  SYM(retro_deinit);
   SYM(retro_load_game);             SYM(retro_unload_game);
   SYM(retro_run);                   SYM(retro_serialize_size);
   SYM(retro_serialize);
#undef SYM

   memset(&gi, 0, sizeof(gi));
   gi.path = "cart.bin";
   gi.data = cart;
   gi.size = TESTROM_CART_SIZE;

   retro_set_environment_(env_cb);
   retro_set_video_refresh_(vid_cb);
   retro_set_audio_sample_(aud_cb);
   retro_set_audio_sample_batch_(audb_cb);
   retro_set_input_poll_(inp_poll);
   retro_set_input_state_(inp_state);

   if (!strcmp(mode, "cycles"))
   {
      unsigned long ref_v = 0, ref_a = 0;
      size_t ref_sz = 0;
      unsigned char *ref_st = NULL;

      for (cyc = 0; cyc < n; cyc++)
      {
         size_t sz;
         unsigned char *st;

         video_hash = audio_hash = 2166136261UL;
         audio_count = audio_batches = 0;
         retro_init_();
         if (!retro_load_game_(&gi))
         {
            puts("FAIL: retro_load_game");
            return 2;
         }
         for (i = 0; i < 90; i++)
            retro_run_();
         sz = retro_serialize_size_();
         st = (unsigned char *)malloc(sz);
         retro_serialize_(st, sz);

         printf("cycle %d: video=%08lx audio=%08lx state=%zu bytes\n",
                cyc, video_hash & 0xffffffffUL, audio_hash & 0xffffffffUL, sz);

         if (cyc == 0)
         {
            ref_v = video_hash; ref_a = audio_hash;
            ref_sz = sz; ref_st = st;
         }
         else
         {
            if (video_hash != ref_v)
            { puts("FAIL: video diverges across lifecycle cycles"); rc = 1; }
            if (audio_hash != ref_a)
            { puts("FAIL: audio diverges across lifecycle cycles"); rc = 1; }
            if (sz != ref_sz || memcmp(st, ref_st, sz))
            {
               size_t j;
               for (j = 0; j < sz && j < ref_sz && st[j] == ref_st[j]; j++)
                  ;
               printf("FAIL: state diverges across lifecycle cycles at byte %zu\n", j);
               rc = 1;
            }
            free(st);
         }
         retro_unload_game_();
         retro_deinit_();
      }
      free(ref_st);
      if (!rc)
         printf("PASS: %d lifecycle cycles identical\n", n);
   }
   else if (!strcmp(mode, "reload"))
   {
      retro_init_();
      for (cyc = 0; cyc < n; cyc++)
      {
         if (!retro_load_game_(&gi))
         {
            puts("FAIL: retro_load_game");
            return 2;
         }
         for (i = 0; i < 30; i++)
            retro_run_();
         retro_unload_game_();
      }
      retro_deinit_();
      printf("PASS: %d reload cycles (leak check via LeakSanitizer at exit)\n", n);
   }
   else /* soak */
   {
      long per_frame = -1;
      retro_init_();
      if (!retro_load_game_(&gi))
      {
         puts("FAIL: retro_load_game");
         return 2;
      }
      for (i = 0; i < n; i++)
      {
         audio_count = 0;
         audio_batches = 0;
         retro_run_();
         if (audio_batches != 1)
         {
            printf("FAIL: %ld audio batches in one retro_run (want 1)\n",
                   audio_batches);
            rc = 1;
            break;
         }
         if (per_frame < 0)
            per_frame = audio_count;
         else if (audio_count != per_frame)
         {
            printf("FAIL: audio frames per retro_run changed (%ld -> %ld)\n",
                   per_frame, audio_count);
            rc = 1;
            break;
         }
      }
      retro_unload_game_();
      retro_deinit_();
      if (!rc)
         printf("PASS: soak %d frames, one batch of %ld audio frames per retro_run\n",
                n, per_frame);
   }

   return rc;
}
