#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#pragma pack(1)
#endif

#include <libretro.h>
#include "libretro_core_options.h"

#include "audio.h"
#include "config.h"
#include "cpu.h"
#include "keyboard.h"
#include "score.h"
#include "vdc.h"
#include "vmachine.h"
#include "voice.h"
#include "vpp.h"
#include "vkeyb/vkeyb.h"

#include "wrapalleg.h"

#ifdef HAVE_VOICE
#include <core_audio_mixer.h>
#include <audio/conversion/float_to_s16.h>
#include <audio/conversion/s16_to_float.h>
#endif
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <encodings/crc32.h>
#include <retro_miscellaneous.h>

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

static bool libretro_supports_bitmasks = false;

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_destroybmp(void);

char bios_file_name[16] = {0};

uint16_t mbmp[TEX_WIDTH * TEX_HEIGHT];
uint16_t *mbmp_prev = NULL;
static bool crop_overscan = false;
/* Note: 320x240 is not really correct,
 * since the actual overscan region varies
 * per system and is not strictly defined.
 * Cropping to 320x240 does however remove
 * any glitchy border content in most games,
 * and it optimises the display for devices
 * with a native 320x240 resolution (this can
 * have a significant beneficial performance
 * impact) */
#define CROPPED_WIDTH  320
#define CROPPED_HEIGHT 240
/* 9 is not a typo. The emulated display
 * is offset by 1 pixel, so we only crop
 * 9 pixels from the left, not 10.
 * (This means we lose one row of pixels
 * from the active area of the virtual
 * keyboard when cropping is enabled,
 * but it does not affect usability
 * in any meaningful way) */
#define CROPPED_OFFSET_X 9
#define CROPPED_OFFSET_Y 5

/* The internal emulation code produces exactly
 * 1056 samples per frame. This leads to sample
 * rates of 63360 Hz for NTSC content and 52800 Hz
 * for PAL content. Unfortunately, these rates are
 * so high and so peculiar that the sinc resampler
 * (used for The Voice, and for general resampling
 * in the RetroArch frontend) performs abominably,
 * creating unacceptable levels of noise and
 * distortion. We therefore only use two thirds
 * of the internally generated samples (704 per
 * frame), which reduces NTSC/PAL sample rates to
 * 42240/35200 Hz. This greatly improves resampling
 * performance and final output sound quality. */
#define AUDIO_SAMPLES_PER_FRAME ((SOUND_BUFFER_LEN * 2) / 3)
#define AUDIO_SAMPLERATE ((evblclk == EVBLCLK_NTSC) ? (AUDIO_SAMPLES_PER_FRAME * 60) : (AUDIO_SAMPLES_PER_FRAME * 50))

uint8_t soundBuffer[SOUND_BUFFER_LEN];
static int16_t audioOutBuffer[AUDIO_SAMPLES_PER_FRAME * 2];
static int16_t audio_volume   = 50;
static float voice_volume     = 0.7f;
static bool low_pass_enabled  = false;
static int32_t low_pass_range = 0;
static int32_t low_pass_prev  = 0;

int RLOOP=0;
int joystick_data[2][5]={{0,0,0,0,0},{0,0,0,0,0}};
static uint8_t p1_index = 0;
static uint8_t p2_index = 1;

int contax, o2flag, g74flag, c52flag, jopflag, helpflag;

extern uint8_t ram[];

/* True if the virtual keyboard must be showed */
static bool vkb_show = false;

struct ButtonsState
{
  bool up, down, right, left;
  bool select, start;
  bool b, y;
};

/* Last state of the buttons for joypad 1 */
struct ButtonsState last_btn_state = { false, false, false, false,
                                       false, false,
                                       false, false };

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;

   environ_cb = cb;
   libretro_set_core_options(environ_cb);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);
}

static bool load_bios(const char *biosname)
{
   RFILE *bios_file = NULL;
   int64_t bytes_read;
   uint32_t crc;
   size_t i;

   if (!biosname)
      return false;

   bios_file = filestream_open(biosname,
         RETRO_VFS_FILE_ACCESS_READ,
         RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!bios_file)
   {
      log_cb(RETRO_LOG_ERROR, "[O2EM]: Error loading BIOS ROM (%s).\n", biosname);
      return false;
   }

   bytes_read = filestream_read(bios_file, rom_table[0], 1024);
   filestream_close(bios_file);

   if (bytes_read != 1024)
   {
      log_cb(RETRO_LOG_ERROR, "[O2EM]: Error loading BIOS ROM (%s).\n", biosname);
      return false;
   }

   for (i=1; i<8; i++)
      memcpy(rom_table[i],rom_table[0],1024);

   crc = encoding_crc32(0, rom_table[0], 1024);

   switch (crc)
   {
      case 0x8016A315:
         log_cb(RETRO_LOG_INFO, "[O2EM]: Magnavox Odyssey2 BIOS ROM loaded (G7000 model)\n");
         app_data.vpp  = 0;
         app_data.bios = ROM_O2;
         break;
      case 0xE20A9F41:
         log_cb(RETRO_LOG_INFO, "[O2EM]: Philips Videopac+ European BIOS ROM loaded (G7400 model)\n");
         app_data.vpp  = 1;
         app_data.bios = ROM_G7400;
         break;
      case 0xA318E8D6:
         if (!((!o2flag)&&(c52flag)))
            log_cb(RETRO_LOG_INFO, "[O2EM]: Philips Videopac+ French BIOS ROM loaded (G7000 model)\n");
         app_data.vpp  = 0;
         app_data.bios = ROM_C52;
         break;
      case 0x11647CA5:
         if (g74flag)
            log_cb(RETRO_LOG_INFO, "[O2EM]: Philips Videopac+ French BIOS ROM loaded (G7400 model)\n");
         app_data.vpp  = 1;
         app_data.bios = ROM_JOPAC;
         break;
      default:
         log_cb(RETRO_LOG_INFO, "[O2EM]: BIOS ROM loaded (unknown version)\n");
         app_data.vpp  = 0;
         app_data.bios = ROM_UNKNOWN;
         break;
   }

   return true;
}

static bool load_cart(const uint8_t *data, size_t size)
{
   int i, nb;

   /* Get ROM CRC */
   app_data.crc = encoding_crc32(0, data, size);

   if (app_data.crc == 0xAFB23F89)
      app_data.exrom = 1;  /* Musician */
   if (app_data.crc == 0x3BFEF56B)
      app_data.exrom = 1;  /* Four in 1 Row! */
   if (app_data.crc == 0x9B5E9356)
      app_data.exrom = 1;  /* Four in 1 Row! (french) */

   if (((app_data.crc == 0x975AB8DA) || (app_data.crc == 0xE246A812)))
   {
      log_cb(RETRO_LOG_ERROR, "[O2EM]: Loaded content is an incomplete ROM dump.\n");
      return false;
   }

   if ((size % 1024) != 0)
   {
      log_cb(RETRO_LOG_ERROR, "[O2EM]: Error: Loaded content is an invalid ROM dump.\n");
      return false;
   }

   /* special MegaCART design by Soeren Gust */
   if ((size ==   32768) ||
       (size ==   65536) ||
       (size ==  131072) ||
       (size ==  262144) ||
       (size ==  524288) ||
       (size == 1048576))
   {
      app_data.megaxrom = 1;
      app_data.bank     = 1;
      megarom           = malloc(1048576);

      if (!megarom)
      {
         log_cb(RETRO_LOG_ERROR, "[O2EM]: Out of memory while processing loaded content.\n");
         return false;
      }

      memcpy(megarom, data, size);

      /* mirror shorter files into full megabyte */
      if (size <   65536)
         memcpy(megarom +  32768, megarom,  32768);
      if (size <  131072)
         memcpy(megarom +  65536, megarom,  65536);
      if (size <  262144)
         memcpy(megarom + 131072, megarom, 131072);
      if (size <  524288)
         memcpy(megarom + 262144, megarom, 262144);
      if (size < 1048576)
         memcpy(megarom + 524288, megarom, 524288);

      /* start in bank 0xff */
      memcpy(&rom_table[0][1024], megarom + 4096*255 + 1024, 3072);

      log_cb(RETRO_LOG_INFO, "[O2EM]: MegaCart %luK\n", (unsigned long)(size / 1024));
      nb = 1;
   }
   else if (((size % 3072) == 0))
   {
      app_data.three_k = 1;
      nb               = size / 3072;

      for (i = (nb - 1); i >= 0; i--)
      {
         memcpy(&rom_table[i][1024], data, 3072);
         data += 3072;
      }

      log_cb(RETRO_LOG_INFO, "[O2EM]: %uK\n", (unsigned)(nb * 3));
   }
   else
   {
      nb = size / 2048;

      if ((nb == 2) && (app_data.exrom))
      {
         memcpy(&extROM[0], data, 1024);
         data += 1024;

         memcpy(&rom_table[0][1024], data, 3072);
         data += 3072;

         log_cb(RETRO_LOG_INFO, "[O2EM]: 3K EXROM\n");
      }
      else
      {
         for (i = (nb - 1); i >= 0; i--)
         {
            memcpy(&rom_table[i][1024], data, 2048);
            data += 2048;

            /* simulate missing A10 */
            memcpy(&rom_table[i][3072], &rom_table[i][2048], 1024);
         }

         log_cb(RETRO_LOG_INFO, "[O2EM]: %uK\n", (unsigned)(nb * 2));
      }
   }

   rom = rom_table[0];
   if (nb == 1)
      app_data.bank = 1;
   else if (nb == 2)
      app_data.bank = app_data.exrom ? 1 : 2;
   else if (nb == 4)
      app_data.bank = 3;
   else
      app_data.bank = 4;

   if ((rom_table[nb-1][1024+12]=='O') &&
       (rom_table[nb-1][1024+13]=='P') &&
       (rom_table[nb-1][1024+14]=='N') &&
       (rom_table[nb-1][1024+15]=='B'))
      app_data.openb=1;

   return true;
}

void update_joy(void)
{
}

static void pointerToScreenCoordinates(int *x, int *y)
{
   if (crop_overscan)
   {
      *x = ((*x + 0x7FFF) * CROPPED_WIDTH / 0xFFFF)  + CROPPED_OFFSET_X;
      *y = ((*y + 0x7FFF) * CROPPED_HEIGHT / 0xFFFF) + CROPPED_OFFSET_Y;
   }
   else
   {
      *x = (*x + 0x7FFF) * EMUWIDTH / 0xFFFF;
      *y = (*y + 0x7FFF) * EMUHEIGHT / 0xFFFF;
   }
}

static void update_input_virtual_keyboard(unsigned joypad_bits)
{
  bool select = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT)) >> RETRO_DEVICE_ID_JOYPAD_SELECT;
  bool start  = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START)) >> RETRO_DEVICE_ID_JOYPAD_START;
  bool y      = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_Y)) >> RETRO_DEVICE_ID_JOYPAD_Y;
  bool up     = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) >> RETRO_DEVICE_ID_JOYPAD_UP;
  bool down   = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) >> RETRO_DEVICE_ID_JOYPAD_DOWN;
  bool left   = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) >> RETRO_DEVICE_ID_JOYPAD_LEFT;
  bool right  = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) >> RETRO_DEVICE_ID_JOYPAD_RIGHT;
  bool b      = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B)) >> RETRO_DEVICE_ID_JOYPAD_B;
  bool click  = input_state_cb(2, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);

  /* Show the virtual keyboard? */
  if (select && !last_btn_state.select)
  {
     vkb_show = !vkb_show;
     /* Release current key when virtual keyboard hidden */
     if (!vkb_show)
        key[vkb_get_current_key_scancode()] = false;
  }

  if (vkb_show)
  {
     /* Move keyboard */
     if (y && !last_btn_state.y)
        vkb_set_virtual_keyboard_position((vkb_get_virtual_keyboard_position() + 1) % 2);

     /* Direct click on the keyboard (touch screen) */
     if (click)
     {
        int xpointer, ypointer;
        xpointer = input_state_cb(2, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
        ypointer = input_state_cb(2, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
        pointerToScreenCoordinates(&xpointer, &ypointer);

        /* A key was touched: act as if the "Action" button was pushed */
        if (vkb_move_at(xpointer, ypointer))
           b = true;
     }

     /* Press key */
     if ((b && !last_btn_state.b) || (!b && last_btn_state.b))
        key[vkb_get_current_key_scancode()] = b;

     if (!b)
     {
        /* Move current key */
        if (right && !last_btn_state.right)
           vkb_move_key(VKB_MOVE_RIGHT);
        else if (left && !last_btn_state.left)
           vkb_move_key(VKB_MOVE_LEFT);
        else if (down && !last_btn_state.down)
           vkb_move_key(VKB_MOVE_DOWN);
        else if (up && !last_btn_state.up)
           vkb_move_key(VKB_MOVE_UP);

        /* If start is pressed than press the Enter key */
        if ((start && !last_btn_state.start) || (!start && last_btn_state.start))
           key[RETROK_RETURN] = start;
     }
  }

  last_btn_state.select = select;
  last_btn_state.start  = start;
  last_btn_state.y      = y;
  last_btn_state.b      = b;
  last_btn_state.left   = left;
  last_btn_state.right  = right;
  last_btn_state.up     = up;
  last_btn_state.down   = down;
}

static void update_input(void)
{
   unsigned joypad_bits[2] = {0};
   size_t i, j;

   if (!input_poll_cb)
      return;

   input_poll_cb();

   if (libretro_supports_bitmasks)
   {
      for (i = 0; i < 2; i++)
      {
         joypad_bits[i] = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);

         /* If virtual KBD is shown, only need input
          * from port 1 */
         if (vkb_show)
            goto done;
      }
   }
   else
   {
      for (i = 0; i < 2; i++)
      {
         for (j = 0; j < (RETRO_DEVICE_ID_JOYPAD_R3+1); j++)
            joypad_bits[i] |= input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, j) ? (1 << j) : 0;

         /* If virtual KBD is shown, only need input
          * from port 1 */
         if (vkb_show)
            goto done;
      }
   }

   if (!vkb_show)
   {
      /* Joystick
       * Player 1 */
      joystick_data[p1_index][0]= (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) >> RETRO_DEVICE_ID_JOYPAD_UP;
      joystick_data[p1_index][1]= (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) >> RETRO_DEVICE_ID_JOYPAD_DOWN;
      joystick_data[p1_index][2]= (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) >> RETRO_DEVICE_ID_JOYPAD_LEFT;
      joystick_data[p1_index][3]= (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) >> RETRO_DEVICE_ID_JOYPAD_RIGHT;
      joystick_data[p1_index][4]= (joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B)) >> RETRO_DEVICE_ID_JOYPAD_B; /* "Action" button on the joystick */
      /* Player 2 */
      joystick_data[p2_index][0]= (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) >> RETRO_DEVICE_ID_JOYPAD_UP;
      joystick_data[p2_index][1]= (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) >> RETRO_DEVICE_ID_JOYPAD_DOWN;
      joystick_data[p2_index][2]= (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) >> RETRO_DEVICE_ID_JOYPAD_LEFT;
      joystick_data[p2_index][3]= (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) >> RETRO_DEVICE_ID_JOYPAD_RIGHT;
      joystick_data[p2_index][4]= (joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B)) >> RETRO_DEVICE_ID_JOYPAD_B; /* "Action" button on the joystick */

      /* Numeric and Alpha */
      key[RETROK_0] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_0) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_X)) >> RETRO_DEVICE_ID_JOYPAD_X);
      key[RETROK_1] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_1) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L)) >> RETRO_DEVICE_ID_JOYPAD_L);
      key[RETROK_2] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_2) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R)) >> RETRO_DEVICE_ID_JOYPAD_R);
      key[RETROK_3] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_3) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L2)) >> RETRO_DEVICE_ID_JOYPAD_L2);
      key[RETROK_4] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_4) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R2)) >> RETRO_DEVICE_ID_JOYPAD_R2);
      key[RETROK_5] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_5) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L3)) >> RETRO_DEVICE_ID_JOYPAD_L3);
      key[RETROK_6] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_6) |
         ((joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R3)) >> RETRO_DEVICE_ID_JOYPAD_R3);

      key[RETROK_7] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_7);
      key[RETROK_8] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_8);
      key[RETROK_9] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_9);
      key[RETROK_a] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_a);
      key[RETROK_b] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_b);
      key[RETROK_c] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_c);
      key[RETROK_d] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_d);
      key[RETROK_e] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_e);
      key[RETROK_f] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_f);
      key[RETROK_g] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_g);
      key[RETROK_h] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_h);
      key[RETROK_i] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_i);
      key[RETROK_j] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_j);
      key[RETROK_k] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_k);
      key[RETROK_l] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_l);
      key[RETROK_m] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_m);
      key[RETROK_n] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_n);
      key[RETROK_o] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_o);
      key[RETROK_p] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_p);
      key[RETROK_q] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_q);
      key[RETROK_r] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_r);
      key[RETROK_s] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_s);
      key[RETROK_t] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_t);
      key[RETROK_u] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_u);
      key[RETROK_v] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_v);
      key[RETROK_w] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_w);
      key[RETROK_x] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x);
      key[RETROK_y] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_y);
      key[RETROK_z] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_z);
      key[RETROK_SPACE]    = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SPACE);       /* Space */
      key[RETROK_QUESTION] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_QUESTION); /* ? */
      key[RETROK_PERIOD]   = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_PERIOD);     /* . */
      key[RETROK_DELETE]   = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_END);           /* "Clear" */
      key[RETROK_RETURN]   = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN);     /* "Enter" */
      key[RETROK_MINUS]    = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_MINUS);       /* - */
      key[RETROK_ASTERISK] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ASTERISK); /* Multiply sign */
      key[RETROK_SLASH]    = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_SLASH);       /* Divide sign */
      key[RETROK_EQUALS]   = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_EQUALS);     /* = */
      key[RETROK_PLUS]     = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_PLUS);         /* + */
   }

done:
   /* Virtual keyboard management */
   update_input_virtual_keyboard(joypad_bits[0]);

   /* Take into account RESET being pressed (F5 is O2EM original setting )*/
   if (key[RETROK_F5] || input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F5))
   {
       retro_reset();
       key[RETROK_F5] = 0;
   }	
}

static void upate_audio(void)
{
   uint8_t *audio_samples_ptr = soundBuffer;
   int16_t *audio_out_ptr     = audioOutBuffer;
   size_t i;

   /* Convert 8u mono to 16s stereo */
   if (low_pass_enabled)
   {
      /* Restore previous sample */
      int32_t low_pass = low_pass_prev;
      /* Single-pole low-pass filter (6 dB/octave) */
      int32_t factor_a = low_pass_range;
      int32_t factor_b = 0x10000 - factor_a;

      for(i = 1; i <= SOUND_BUFFER_LEN; i++)
      {
         int16_t sample16;

         /* For improved resampler performance, we
          * reduce the internal sample rate to 2/3
          * of its original value. Odyssey2/Videopac
          * audio is so primitive that we can simply
          * skip every third sample with no perceivable
          * reduction in sound quality. */
         if ((i % 3) == 0)
         {
            audio_samples_ptr++;
            continue;
         }

         /* Get current sample */
         sample16  = (((*(audio_samples_ptr++) * audio_volume) /
               100) - 128) << 8;

         /* Apply low-pass filter */
         low_pass = (low_pass * factor_a) + (sample16 * factor_b);

         /* 16.16 fixed point */
         low_pass >>= 16;

         /* Update output buffer */
         *(audio_out_ptr++) = (int16_t)low_pass;
         *(audio_out_ptr++) = (int16_t)low_pass;
      }

      /* Save last sample for next frame */
      low_pass_prev = low_pass;
   }
   else
   {
      for(i = 1; i <= SOUND_BUFFER_LEN; i++)
      {
         int16_t sample16;

         if ((i % 3) == 0)
         {
            audio_samples_ptr++;
            continue;
         }

         /* Get current sample */
         sample16  = (((*(audio_samples_ptr++) * audio_volume) /
               100) - 128) << 8;

         *(audio_out_ptr++) = (int16_t)sample16;
         *(audio_out_ptr++) = (int16_t)sample16;
      }
   }

#ifdef HAVE_VOICE
   if (get_voice_status())
   {
      float fbuf[AUDIO_SAMPLES_PER_FRAME * 2];

      /* Convert output audio buffer to float */
      convert_s16_to_float(fbuf, audioOutBuffer, AUDIO_SAMPLES_PER_FRAME * 2, 1.0f);

      /* Mix in voice */
      core_audio_mixer_mix(fbuf, AUDIO_SAMPLES_PER_FRAME, voice_volume, true);

      /* Convert back to signed integer */
      convert_float_to_s16(audioOutBuffer, fbuf, AUDIO_SAMPLES_PER_FRAME * 2);
   }
#endif

   audio_batch_cb(audioOutBuffer, AUDIO_SAMPLES_PER_FRAME);
}

/************************************
 * Interframe blending
 ************************************/

enum frame_blend_method
{
   FRAME_BLEND_NONE = 0,
   FRAME_BLEND_MIX,
   FRAME_BLEND_GHOST_65,
   FRAME_BLEND_GHOST_75,
   FRAME_BLEND_GHOST_85,
   FRAME_BLEND_GHOST_95
};

/* It would be more flexible to have 'persistence'
 * as a core option, but using a variable parameter
 * reduces performance by ~15%. We therefore offer
 * fixed values, and use a macro to avoid excessive
 * duplication of code... (and yet we still have to
 * duplicate code due to the annoying SUPPORT_ABGR1555
 * ifdefs...)
 * Note: persistence fraction is (persistence/128),
 * using a power of 2 like this further increases
 * performance by ~15% */
#if defined(SUPPORT_ABGR1555)
#define BLEND_FRAMES_GHOST(persistence)                                                               \
{                                                                                                     \
   uint16_t *curr = mbmp;                                                                             \
   uint16_t *prev = mbmp_prev;                                                                        \
   size_t x, y;                                                                                       \
                                                                                                      \
   for (y = 0; y < EMUHEIGHT; y++)                                                                    \
   {                                                                                                  \
      for (x = 0; x < EMUWIDTH; x++)                                                                  \
      {                                                                                               \
         /* Get colours from current + previous frames */                                             \
         uint16_t color_curr = *(curr);                                                               \
         uint16_t color_prev = *(prev);                                                               \
                                                                                                      \
         /* Unpack colours */                                                                         \
         uint16_t r_curr     = (color_curr      ) & 0x1F;                                             \
         uint16_t g_curr     = (color_curr >>  5) & 0x1F;                                             \
         uint16_t b_curr     = (color_curr >> 10) & 0x1F;                                             \
                                                                                                      \
         uint16_t r_prev     = (color_curr      ) & 0x1F;                                             \
         uint16_t g_prev     = (color_curr >>  5) & 0x1F;                                             \
         uint16_t b_prev     = (color_curr >> 10) & 0x1F;                                             \
                                                                                                      \
         /* Mix colors */                                                                             \
         uint16_t r_mix      = ((r_curr * (128 - persistence)) >> 7) + ((r_prev * persistence) >> 7); \
         uint16_t g_mix      = ((g_curr * (128 - persistence)) >> 7) + ((g_prev * persistence) >> 7); \
         uint16_t b_mix      = ((b_curr * (128 - persistence)) >> 7) + ((b_prev * persistence) >> 7); \
                                                                                                      \
         /* Output colour is the maximum of the input                                                 \
          * and decayed values */                                                                     \
         uint16_t r_out      = (r_mix > r_curr) ? r_mix : r_curr;                                     \
         uint16_t g_out      = (g_mix > g_curr) ? g_mix : g_curr;                                     \
         uint16_t b_out      = (b_mix > b_curr) ? b_mix : b_curr;                                     \
         uint16_t color_out  = b_out << 10 | g_out << 5 | r_out;                                      \
                                                                                                      \
         /* Assign colour and store for next frame */                                                 \
         *(prev++)           = color_out;                                                             \
         *(curr++)           = color_out;                                                             \
      }                                                                                               \
                                                                                                      \
      curr += (TEX_WIDTH - EMUWIDTH);                                                                 \
      prev += (TEX_WIDTH - EMUWIDTH);                                                                 \
   }                                                                                                  \
}
#else
#define BLEND_FRAMES_GHOST(persistence)                                                               \
{                                                                                                     \
   uint16_t *curr = mbmp;                                                                             \
   uint16_t *prev = mbmp_prev;                                                                        \
   size_t x, y;                                                                                       \
                                                                                                      \
   for (y = 0; y < EMUHEIGHT; y++)                                                                    \
   {                                                                                                  \
      for (x = 0; x < EMUWIDTH; x++)                                                                  \
      {                                                                                               \
         /* Get colours from current + previous frames */                                             \
         uint16_t color_curr = *(curr);                                                               \
         uint16_t color_prev = *(prev);                                                               \
                                                                                                      \
         /* Unpack colours */                                                                         \
         uint16_t r_curr     = (color_curr >> 11) & 0x1F;                                             \
         uint16_t g_curr     = (color_curr >>  6) & 0x1F;                                             \
         uint16_t b_curr     = (color_curr      ) & 0x1F;                                             \
                                                                                                      \
         uint16_t r_prev     = (color_prev >> 11) & 0x1F;                                             \
         uint16_t g_prev     = (color_prev >>  6) & 0x1F;                                             \
         uint16_t b_prev     = (color_prev      ) & 0x1F;                                             \
                                                                                                      \
         /* Mix colors */                                                                             \
         uint16_t r_mix      = ((r_curr * (128 - persistence)) >> 7) + ((r_prev * persistence) >> 7); \
         uint16_t g_mix      = ((g_curr * (128 - persistence)) >> 7) + ((g_prev * persistence) >> 7); \
         uint16_t b_mix      = ((b_curr * (128 - persistence)) >> 7) + ((b_prev * persistence) >> 7); \
                                                                                                      \
         /* Output colour is the maximum of the input                                                 \
          * and decayed values */                                                                     \
         uint16_t r_out      = (r_mix > r_curr) ? r_mix : r_curr;                                     \
         uint16_t g_out      = (g_mix > g_curr) ? g_mix : g_curr;                                     \
         uint16_t b_out      = (b_mix > b_curr) ? b_mix : b_curr;                                     \
         uint16_t color_out  = r_out << 11 | g_out << 6 | b_out;                                      \
                                                                                                      \
         /* Assign colour and store for next frame */                                                 \
         *(prev++)           = color_out;                                                             \
         *(curr++)           = color_out;                                                             \
      }                                                                                               \
                                                                                                      \
      curr += (TEX_WIDTH - EMUWIDTH);                                                                 \
      prev += (TEX_WIDTH - EMUWIDTH);                                                                 \
   }                                                                                                  \
}
#endif

static void blend_frames_mix(void)
{
   uint16_t *curr = mbmp;
   uint16_t *prev = mbmp_prev;
   size_t x, y;

   for (y = 0; y < EMUHEIGHT; y++)
   {
      for (x = 0; x < EMUWIDTH; x++)
      {
         /* Get colours from current + previous frames */
         uint16_t color_curr = *(curr);
         uint16_t color_prev = *(prev);

         /* Store colours for next frame */
         *(prev++) = color_curr;

         /* Mix colours */
   #if defined(SUPPORT_ABGR1555)
         *(curr++) = (color_curr + color_prev + ((color_curr ^ color_prev) & 0x521)) >> 1;
   #else
         *(curr++) = (color_curr + color_prev + ((color_curr ^ color_prev) & 0x821)) >> 1;
   #endif
      }

      curr += (TEX_WIDTH - EMUWIDTH);
      prev += (TEX_WIDTH - EMUWIDTH);
   }
}

static void blend_frames_ghost65(void)
{
   /* 65% = 83 / 128 */
   BLEND_FRAMES_GHOST(83);
}

static void blend_frames_ghost75(void)
{
   /* 75% = 95 / 128 */
   BLEND_FRAMES_GHOST(95);
}

static void blend_frames_ghost85(void)
{
   /* 85% ~= 109 / 128 */
   BLEND_FRAMES_GHOST(109);
}

static void blend_frames_ghost95(void)
{
   /* 95% ~= 122 / 128 */
   BLEND_FRAMES_GHOST(122);
}

static void (*blend_frames)(void) = NULL;

static void init_frame_blending(enum frame_blend_method blend_method)
{
   /* Allocate/zero out buffer, if required */
   if (blend_method != FRAME_BLEND_NONE)
   {
      if (!mbmp_prev)
         mbmp_prev = (uint16_t*)malloc(TEX_WIDTH * TEX_HEIGHT * sizeof(uint16_t));

      memset(mbmp_prev, 0, TEX_WIDTH * TEX_HEIGHT * sizeof(uint16_t));
   }

   /* Assign function pointer */
   switch (blend_method)
   {
      case FRAME_BLEND_MIX:
         blend_frames = blend_frames_mix;
         break;
      case FRAME_BLEND_GHOST_65:
         blend_frames = blend_frames_ghost65;
         break;
      case FRAME_BLEND_GHOST_75:
         blend_frames = blend_frames_ghost75;
         break;
      case FRAME_BLEND_GHOST_85:
         blend_frames = blend_frames_ghost85;
         break;
      case FRAME_BLEND_GHOST_95:
         blend_frames = blend_frames_ghost95;
         break;
      default:
         blend_frames = NULL;
         break;
   }
}

/************************************
 * libretro implementation
 ************************************/

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
	info->library_name = "O2EM";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
	info->library_version = "1.18" GIT_VERSION;
	info->need_fullpath = false;
	info->valid_extensions = "bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   info->timing.fps               = (evblclk == EVBLCLK_NTSC) ? 60 : 50;
   info->timing.sample_rate       = AUDIO_SAMPLERATE;

   if (crop_overscan)
   {
      info->geometry.base_width   = CROPPED_WIDTH;
      info->geometry.base_height  = CROPPED_HEIGHT;
   }
   else
   {
      info->geometry.base_width   = EMUWIDTH;
      info->geometry.base_height  = EMUHEIGHT;
   }

   info->geometry.max_width       = EMUWIDTH;
   info->geometry.max_height      = EMUHEIGHT;
   info->geometry.aspect_ratio    = 4.0f / 3.0f;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

size_t retro_serialize_size(void) 
{ 
	return savestate_size();
}

bool retro_serialize(void *data, size_t size)
{
   return savestate_to_mem((uint8_t *)data, size);
}

bool retro_unserialize(const void *data, size_t size)
{
   return loadstate_from_mem((uint8_t *)data, size);
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
   char bios_file_path[PATH_MAX_LENGTH];
   const uint8_t *rom_data              = NULL;
   size_t rom_size                      = 0;
   const char *system_directory_c       = NULL;
   enum retro_pixel_format fmt          = RETRO_PIXEL_FORMAT_RGB565;
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Action" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Move Virtual Keyboard" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Show/Hide Virtual Keyboard" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Numeric Key 0" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Numeric Key 1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Numeric Key 2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Numeric Key 3" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Numeric Key 4" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Numeric Key 5" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Numeric Key 6" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Action" },

      { 2, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X, "Virtual Keyboard: Pointer X" },
      { 2, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y, "Virtual Keyboard: Pointer Y" },
      { 2, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED, "Virtual Keyboard: Pointer Pressed" },

      { 0 }
   };

   if (!info || !info->data || (info->size < 1))
      return false;

   rom_data = (const uint8_t *)info->data;
   rom_size = info->size;

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "[O2EM]: RGB565 is not supported.\n");
      return false;
   }

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   /* BIOS is required */
   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_directory_c);
   if (!system_directory_c)
   {
      log_cb(RETRO_LOG_WARN, "[O2EM]: no system directory defined, unable to look for %s\n", bios_file_name);
      return false;
   }

   fill_pathname_join(bios_file_path, system_directory_c, bios_file_name, sizeof(bios_file_path));
   if (!path_is_valid(bios_file_path))
   {
      log_cb(RETRO_LOG_WARN, "[O2EM]: %s not found, cannot load BIOS\n", bios_file_name);
      return false;
   }

   app_data.stick[0] = app_data.stick[1] = 1;
   app_data.sticknumber[0] = app_data.sticknumber[1] = 0;
   set_defjoykeys(0,0);
   set_defjoykeys(1,1);
   set_defsystemkeys();
   app_data.bank = 0;
   app_data.limit = 1;
   app_data.sound_en = 1;
   app_data.speed = 100;
   app_data.wsize = 2;
   app_data.scanlines = 0;
   app_data.voice = 1;
   /* These volume settings have no effect
    * (they are allegro-specific) */
   app_data.svolume = 100;
   app_data.vvolume = 100;
   /* Internal audio filter is worthless,
    * disable it and use our own */
   app_data.filter = 0;
   app_data.exrom = 0;
   app_data.three_k = 0;
   app_data.crc = 0;
   app_data.openb = 0;
   app_data.vpp = 0;
   app_data.bios = 0;
   app_data.scoretype = 0;
   app_data.scoreaddress = 0;
   app_data.default_highscore = 0;
   app_data.breakpoint = 65535;
   app_data.megaxrom = 0;

   init_audio();

   o2flag = 1;

   if (!load_bios(bios_file_path))
      return false;
   if (!load_cart(rom_data, rom_size))
      return false;

   init_display();
   init_cpu();
   init_system();

#ifdef HAVE_VOICE
   if (app_data.voice)
   {
      char voice_path[PATH_MAX_LENGTH];
      voice_path[0] = '\0';
      core_audio_mixer_init(AUDIO_SAMPLERATE);
      fill_pathname_join(voice_path, system_directory_c, "voice", sizeof(voice_path));
      init_voice(voice_path);
   }
#endif

   set_score(app_data.scoretype, app_data.scoreaddress, app_data.default_highscore);

   return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{
}

unsigned retro_get_region(void)
{
    return evblclk == EVBLCLK_NTSC ? RETRO_REGION_NTSC : RETRO_REGION_PAL;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return ram;
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return 64 + 256;
    return 0;
}

static void check_variables(bool startup)
{
   struct retro_variable var;
   enum VkbAlpha keyboard_alpha;
   enum frame_blend_method blend_method;
   bool last_crop_overscan;

   if (startup)
   {
      bool auto_region = true;

      /* Console Region */
      var.key       = "o2em_region";
      var.value     = NULL;
      app_data.euro = 0;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         if (!strcmp(var.value, "NTSC"))
         {
            app_data.euro = 0;
            auto_region   = false;
         }
         else if (!strcmp(var.value, "PAL"))
         {
            app_data.euro = 1;
            auto_region   = false;
         }
      }

      /* Emulated Hardware */
      var.key   = "o2em_bios";
      var.value = NULL;
      strcpy(bios_file_name, "o2rom.bin");

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         /* Ugly, but we don't want to inadvertently
          * copy an invalid value */
         if (!strcmp(var.value, "c52.bin"))
         {
            strcpy(bios_file_name, "c52.bin");
            if (auto_region)
               app_data.euro = 1;
         }
         else if (!strcmp(var.value, "g7400.bin"))
         {
            strcpy(bios_file_name, "g7400.bin");
            if (auto_region)
               app_data.euro = 1;
         }
         else if (!strcmp(var.value, "jopac.bin"))
         {
            strcpy(bios_file_name, "jopac.bin");
            if (auto_region)
               app_data.euro = 1;
         }
      }
   }

   /* Swap Gamepads */
   var.key   = "o2em_swap_gamepads";
   var.value = NULL;
   p1_index  = 0;
   p2_index  = 1;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
      {
         p1_index = 1;
         p2_index = 0;
      }
   }

   /* Virtual KBD Transparency */
   var.key        = "o2em_vkbd_transparency";
   var.value      = NULL;
   keyboard_alpha = VKB_ALPHA_100;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "25"))
         keyboard_alpha = VKB_ALPHA_75;
      else if (!strcmp(var.value, "50"))
         keyboard_alpha = VKB_ALPHA_50;
      else if (!strcmp(var.value, "75"))
         keyboard_alpha = VKB_ALPHA_25;
   }

   vkb_set_virtual_keyboard_transparency(keyboard_alpha);

   /* Crop Overscan */
   var.key            = "o2em_crop_overscan";
   var.value          = NULL;
   last_crop_overscan = crop_overscan;
   crop_overscan      = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (!strcmp(var.value, "enabled"))
         crop_overscan = true;

   if (!startup && (crop_overscan != last_crop_overscan))
   {
      struct retro_system_av_info av_info;
      retro_get_system_av_info(&av_info);
      environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);
   }

   /* Interframe Blending */
   var.key      = "o2em_mix_frames";
   var.value    = NULL;
   blend_method = FRAME_BLEND_NONE;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "mix"))
         blend_method = FRAME_BLEND_MIX;
      else if (!strcmp(var.value, "ghost_65"))
         blend_method = FRAME_BLEND_GHOST_65;
      else if (!strcmp(var.value, "ghost_75"))
         blend_method = FRAME_BLEND_GHOST_75;
      else if (!strcmp(var.value, "ghost_85"))
         blend_method = FRAME_BLEND_GHOST_85;
      else if (!strcmp(var.value, "ghost_95"))
         blend_method = FRAME_BLEND_GHOST_95;
   }

   init_frame_blending(blend_method);

   /* Audio Volume */
   var.key      = "o2em_audio_volume";
   var.value    = NULL;
   audio_volume = 50;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      audio_volume = atoi(var.value);
      audio_volume = (audio_volume > 100) ? 100 : audio_volume;
      audio_volume = (audio_volume < 0)   ? 0   : audio_volume;
   }

   /* > Since we are not using the internal audio
    *   filter, volume must be divided by a factor
    *   of two */
   audio_volume = audio_volume >> 1;

   /* Voice Volume */
   var.key      = "o2em_voice_volume";
   var.value    = NULL;
   voice_volume = 0.7f;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int16_t volume_level = atoi(var.value);
      volume_level = (volume_level > 100) ? 100 : volume_level;
      volume_level = (volume_level < 0)   ? 0   : volume_level;
      voice_volume = (float)volume_level / 100.0f;
   }

   /* Audio Filter */
   var.key          = "o2em_low_pass_filter";
   var.value        = NULL;
   low_pass_enabled = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (!strcmp(var.value, "enabled"))
         low_pass_enabled = true;

   /* Audio Filter Level */
   var.key        = "o2em_low_pass_range";
   var.value      = NULL;
   low_pass_range = (60 * 0x10000) / 100;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      low_pass_range = (strtol(var.value, NULL, 10) * 0x10000) / 100;
}

static void log_printf_null(enum retro_log_level level,
      const char *fmt, ...) { }

void retro_init(void)
{
   struct retro_log_callback log;
   unsigned level = 5;

   libretro_supports_bitmasks = false;
   crop_overscan              = false;
   vkb_show                   = false;
   low_pass_prev              = 0;
   RLOOP                      = 1;

   memset(mbmp, 0, sizeof(mbmp));

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = log_printf_null;

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   vkb_configure_virtual_keyboard(mbmp, EMUWIDTH, EMUHEIGHT, TEX_WIDTH);
   check_variables(true);
}

void retro_deinit(void)
{
   close_audio();
   close_voice();
   close_display();
   close_vpp();
   retro_destroybmp();

   if (mbmp_prev)
   {
      free(mbmp_prev);
      mbmp_prev = NULL;
   }

#ifdef HAVE_VOICE
   core_audio_mixer_done();
#endif

   if (megarom)
      free(megarom);
   megarom = NULL;
}

void retro_reset(void)
{
   init_cpu();
   init_roms();
   init_vpp();
   clearscr();
}

void retro_run(void)
{
   bool var_updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &var_updated) && var_updated)
     check_variables(false);

   update_input();

   cpu_exec();
   RLOOP=1;

   if (blend_frames)
      blend_frames();

   if (vkb_show)
     vkb_show_virtual_keyboard();

   if (crop_overscan)
   {
		uint16_t *mbmp_cropped = mbmp + (TEX_WIDTH * CROPPED_OFFSET_Y) + CROPPED_OFFSET_X;
		video_cb(mbmp_cropped, CROPPED_WIDTH, CROPPED_HEIGHT, TEX_WIDTH << 1);
   }
   else
      video_cb(mbmp, EMUWIDTH, EMUHEIGHT, TEX_WIDTH << 1);

   upate_audio();
}
