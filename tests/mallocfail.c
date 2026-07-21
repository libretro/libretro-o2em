/* LD_PRELOAD allocation fault injector for OOM regression testing.
 *
 * Fails exactly the Nth allocation whose size is at least MF_SIZE_MIN
 * bytes, counted across malloc and calloc, then behaves normally.
 * Used by CI to verify that the core fails retro_load_game gracefully
 * on every large allocation of the load path instead of terminating
 * the host process.
 *
 * Environment:
 *   MF_FAIL_NTH   1-based index of the qualifying allocation to fail
 *   MF_SIZE_MIN   minimum size in bytes for an allocation to qualify
 *
 * Linux/glibc only; this is a test fixture, not shipped code.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>

static void *(*real_malloc)(size_t);
static void *(*real_calloc)(size_t, size_t);
static long nth = -1, seen = 0, size_min = 0;
static int init_done;

static void mf_init(void)
{
   const char *n = getenv("MF_FAIL_NTH");
   const char *s = getenv("MF_SIZE_MIN");
   real_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
   real_calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
   if (n) nth = atol(n);
   if (s) size_min = atol(s);
   init_done = 1;
}

void *malloc(size_t sz)
{
   if (!init_done)
      mf_init();
   if (nth > 0 && (long)sz >= size_min && ++seen == nth)
      return NULL;
   return real_malloc(sz);
}

void *calloc(size_t n, size_t sz)
{
   /* dlsym itself may calloc before mf_init can resolve the real
    * function; serve those from a small static arena */
   static char boot[4096];
   static size_t used;
   if (!init_done && !real_calloc)
   {
      void *p = boot + used;
      used += n * sz;
      return p;
   }
   if (!init_done)
      mf_init();
   if (nth > 0 && (long)(n * sz) >= size_min && ++seen == nth)
      return NULL;
   return real_calloc(n, sz);
}
