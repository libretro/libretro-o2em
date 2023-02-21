/* Virtual keyboard */

#include "vkeyb.h"
#include "vkeyb_config.h"
#include "vkeyb_layout.h"
#include "ui.h"

#include "odyssey2_keyboard.inc"

static const uint16_t *current_kb_image_data = ODYSSEY2_KEYBOARD_IMG_DATA;
static int current_kb_width = ODYSSEY2_KEYBOARD_IMG_WIDTH;
static int current_kb_height = ODYSSEY2_KEYBOARD_IMG_HEIGHT;
static const struct VKey *current_key = 0;
static enum VkbPosition vkb_position = VKB_POS_DOWN;
static const struct VKey *current_keyboard_layout = o2_kb;
static int current_keyboard_keys = ODYSSEY2_KB_KEYS;

#if defined(SUPPORT_ABGR1555)
/* Hack for PS2 that expects ABGR1555 encoded pixels */
static uint16_t color_select = 0x03FF;
#else
static uint16_t color_select = 0xFFC0;
#endif

static int box_thickness = 2;

void vkb_configure_virtual_keyboard(uint16_t *video_buffer, int width, int height, int pitch)
{
  vkb_video_buffer  = video_buffer;
  vkb_screen_width  = width;
  vkb_screen_height = height;
  vkb_screen_pitch  = pitch;
  current_key       = ODYSSEY2_DEFAULT_KEY;
}

/* Set the virtual keyboard transparency */
void vkb_set_virtual_keyboard_transparency(enum VkbAlpha alpha)
{
  vkb_alpha = alpha;
}

void vkb_set_virtual_keyboard_position(enum VkbPosition position)
{
  vkb_position = position;
}

enum VkbPosition vkb_get_virtual_keyboard_position(void)
{
  return vkb_position;
}

void vkb_show_virtual_keyboard(void)
{
  /* Draw keyboard */
  int keyb_x = (vkb_screen_width - current_kb_width) / 2;
  int keyb_y = (vkb_position == VKB_POS_DOWN) ? vkb_screen_height - current_kb_height : 0;
  draw_bmp(keyb_x, keyb_y, current_kb_image_data, current_kb_width, current_kb_height);

  /* Drawn current position */
  draw_box(keyb_x+current_key->x, keyb_y+current_key->y,
           current_key->width, current_key->height, box_thickness, color_select);
}

void vkb_move_key(enum VkbMoveDirection direction)
{
  switch (direction)
  {
    case VKB_MOVE_LEFT:
      current_key = current_key->left;
      break;
    case VKB_MOVE_RIGHT:
      current_key = current_key->right;
      break;
    case VKB_MOVE_UP:
      current_key = current_key->up;
      break;
    case VKB_MOVE_DOWN:
      current_key = current_key->down;
      break;
  }
}

int vkb_get_current_key_scancode(void)
{
  return current_key->scancode;
}

bool vkb_move_at(int x, int y)
{
  int i;
  y -= ((vkb_position == VKB_POS_DOWN) ? vkb_screen_height - current_kb_height : 0);
  x -= (vkb_screen_width - current_kb_width) / 2;
  for (i = 0; i < current_keyboard_keys; i++)
  {
    const struct VKey *key = &current_keyboard_layout[i];
    if (   (key->x <= x) && (x <= key->x + key->width)
        && (key->y <= y) && (y <= key->y + key->height))
    {
      current_key = key;
      return true;
    }
  }
  return false;
}

