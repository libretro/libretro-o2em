/* Savestate determinism regression test.
 *
 * Loads the built core twice in separate forked processes so each gets
 * pristine library globals: process A boots a synthetic BIOS, runs 120
 * frames, serializes, runs 120 more and serializes again (the
 * "continuous" reference); process B boots fresh, restores A's first
 * state, runs 120 frames and serializes (the "restored" result). The
 * two final states must be byte-identical - any machine state that the
 * savestate format fails to carry shows up as a divergence, because
 * the test program couples the at-risk variables into serialized RAM.
 *
 * The synthetic BIOS is a hand-assembled 8048 program: its timer ISR
 * snapshots the accumulator into internal RAM on every interrupt while
 * the main loop seeds the i8244 sound shift register, enables noise,
 * and free-runs INC A. This exercises the accumulator, the internal
 * timer, interrupt state, cycle counters and the audio shift register
 * write-back path.
 *
 * Usage: savestate_determinism <path/to/o2em_libretro.so>
 * Exit code 0 on success.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include "libretro.h"

#define FRAMES_BEFORE_SAVE 120
#define FRAMES_AFTER_SAVE  120
#define CART_SIZE          2048

static char workdir[256];

/* hand-assembled 8048 test program, placed in a fake 1KB BIOS image:
 *   000: JMP 010                    ; reset vector
 *   007: MOV R0,#20; MOV @R0,A     ; timer ISR: RAM[20h] = ACC
 *        INC R7; RETR
 *   010: EN TCNTI; STRT T
 *        ANL P1,#B7                 ; enable VDC access
 *        MOV R0,#A9; MOV A,#5A; MOVX @R0,A   ; seed shift reg low byte
 *        MOV R0,#AA; MOV A,#9F; MOVX @R0,A   ; enable+noise+volume
 *   01E: INC A; JMP 01E             ; free-running accumulator
 */
static void write_test_images(void)
{
   static const unsigned char isr[]  = { 0xB8, 0x20, 0xA0, 0x1F, 0x93 };
   static const unsigned char main_[] = {
      0x25, 0x55, 0x99, 0xB7,
      0xB8, 0xA9, 0x23, 0x5A, 0x90,
      0xB8, 0xAA, 0x23, 0x9F, 0x90,
      0x17, 0x04, 0x1E
   };
   unsigned char bios[1024];
   unsigned char cart[CART_SIZE];
   char path[512];
   FILE *f;

   memset(bios, 0, sizeof(bios));
   bios[0] = 0x04; bios[1] = 0x10;              /* JMP 010 */
   memcpy(bios + 0x007, isr, sizeof(isr));
   memcpy(bios + 0x010, main_, sizeof(main_));

   snprintf(path, sizeof(path), "%s/o2rom.bin", workdir);
   f = fopen(path, "wb");
   fwrite(bios, 1, sizeof(bios), f);
   fclose(f);

   memset(cart, 0, sizeof(cart));
   snprintf(path, sizeof(path), "%s/cart.bin", workdir);
   f = fopen(path, "wb");
   fwrite(cart, 1, sizeof(cart), f);
   fclose(f);
}

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

static void vid_cb(const void *d, unsigned w, unsigned h, size_t p)
{ (void)d; (void)w; (void)h; (void)p; }
static void aud_cb(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audb_cb(const int16_t *d, size_t f) { (void)d; return f; }
static void inp_poll(void) { }
static int16_t inp_state(unsigned a, unsigned b, unsigned c, unsigned d)
{ (void)a; (void)b; (void)c; (void)d; return 0; }

static void dump(const char *name, const void *buf, size_t sz)
{
   char path[512];
   FILE *f;
   snprintf(path, sizeof(path), "%s/%s", workdir, name);
   f = fopen(path, "wb");
   fwrite(buf, 1, sz, f);
   fclose(f);
}

static size_t slurp(const char *name, void *buf, size_t sz)
{
   char path[512];
   size_t n;
   FILE *f;
   snprintf(path, sizeof(path), "%s/%s", workdir, name);
   f = fopen(path, "rb");
   if (!f)
      return 0;
   n = fread(buf, 1, sz, f);
   fclose(f);
   return n;
}

/* runs inside a forked child: fresh dlopen means fresh core globals */
static int core_process(const char *libpath, int restore_mode)
{
   struct retro_game_info gi;
   unsigned char cart[CART_SIZE];
   unsigned char *buf;
   size_t sz;
   int i;
   void *h = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);

   if (!h)
   {
      fprintf(stderr, "dlopen: %s\n", dlerror());
      return 2;
   }

#define SYM(n) __typeof__(n) *n##_ = dlsym(h, #n)
   SYM(retro_set_environment);       SYM(retro_set_video_refresh);
   SYM(retro_set_audio_sample);      SYM(retro_set_audio_sample_batch);
   SYM(retro_set_input_poll);        SYM(retro_set_input_state);
   SYM(retro_init);                  SYM(retro_load_game);
   SYM(retro_run);                   SYM(retro_serialize_size);
   SYM(retro_serialize);             SYM(retro_unserialize);
#undef SYM

   memset(&gi, 0, sizeof(gi));
   if (slurp("cart.bin", cart, sizeof(cart)) != CART_SIZE)
      return 2;
   gi.path = "cart.bin";
   gi.data = cart;
   gi.size = CART_SIZE;

   retro_set_environment_(env_cb);
   retro_set_video_refresh_(vid_cb);
   retro_set_audio_sample_(aud_cb);
   retro_set_audio_sample_batch_(audb_cb);
   retro_set_input_poll_(inp_poll);
   retro_set_input_state_(inp_state);
   retro_init_();

   if (!retro_load_game_(&gi))
   {
      fprintf(stderr, "retro_load_game failed\n");
      return 2;
   }

   sz  = retro_serialize_size_();
   buf = (unsigned char *)malloc(sz);

   if (restore_mode)
   {
      if (slurp("state.bin", buf, sz) != sz)
      {
         fprintf(stderr, "state size mismatch\n");
         return 2;
      }
      if (!retro_unserialize_(buf, sz))
      {
         fprintf(stderr, "retro_unserialize failed\n");
         return 2;
      }
      for (i = 0; i < FRAMES_AFTER_SAVE; i++)
         retro_run_();
      retro_serialize_(buf, sz);
      dump("restored.bin", buf, sz);
   }
   else
   {
      for (i = 0; i < FRAMES_BEFORE_SAVE; i++)
         retro_run_();
      retro_serialize_(buf, sz);
      dump("state.bin", buf, sz);
      for (i = 0; i < FRAMES_AFTER_SAVE; i++)
         retro_run_();
      retro_serialize_(buf, sz);
      dump("continuous.bin", buf, sz);
   }

   free(buf);
   return 0;
}

static int run_child(const char *libpath, int restore_mode)
{
   int status;
   pid_t pid = fork();

   if (pid == 0)
      _exit(core_process(libpath, restore_mode));
   waitpid(pid, &status, 0);
   return WIFEXITED(status) ? WEXITSTATUS(status) : 2;
}

int main(int argc, char **argv)
{
   unsigned char *a, *b;
   long sz_a, sz_b, i;
   char tmpl[] = "/tmp/o2em_ssdet_XXXXXX";

   if (argc < 2)
   {
      fprintf(stderr, "usage: %s <o2em_libretro.so>\n", argv[0]);
      return 2;
   }
   if (!mkdtemp(tmpl))
      return 2;
   snprintf(workdir, sizeof(workdir), "%s", tmpl);
   write_test_images();

   if (run_child(argv[1], 0) != 0)
      return 2;
   if (run_child(argv[1], 1) != 0)
      return 2;

   {
      char pa[512], pb[512];
      FILE *fa, *fb;
      snprintf(pa, sizeof(pa), "%s/continuous.bin", workdir);
      snprintf(pb, sizeof(pb), "%s/restored.bin", workdir);
      fa = fopen(pa, "rb"); fb = fopen(pb, "rb");
      fseek(fa, 0, SEEK_END); sz_a = ftell(fa); fseek(fa, 0, SEEK_SET);
      fseek(fb, 0, SEEK_END); sz_b = ftell(fb); fseek(fb, 0, SEEK_SET);
      a = (unsigned char *)malloc(sz_a);
      b = (unsigned char *)malloc(sz_b);
      if (fread(a, 1, sz_a, fa) != (size_t)sz_a) return 2;
      if (fread(b, 1, sz_b, fb) != (size_t)sz_b) return 2;
      fclose(fa); fclose(fb);
   }

   if (sz_a != sz_b)
   {
      printf("FAIL: state size differs (%ld vs %ld)\n", sz_a, sz_b);
      return 1;
   }
   for (i = 0; i < sz_a; i++)
   {
      if (a[i] != b[i])
      {
         printf("FAIL: continuous vs restored state diverges at byte %ld "
                "(of %ld): %02X vs %02X\n", i, sz_a, a[i], b[i]);
         return 1;
      }
   }
   printf("PASS: savestate restore is deterministic (%ld bytes compared)\n",
          sz_a);
   return 0;
}
