/* Utility functions to write UI elements on the screen */

#include "ui.h"
#include "vkeyb_config.h"

#if defined(SUPPORT_ABGR1555)
#define BLEND_MASK 0x521
#else
#define BLEND_MASK 0x821
#endif

#define BLEND_ALPHA25(fg, bg, out)                                     \
{                                                                      \
   uint16_t color_50 = ((fg + bg + ((fg ^ bg) & BLEND_MASK)) >> 1);    \
   (*(out)) = ((color_50 + bg + ((color_50 ^ bg) & BLEND_MASK)) >> 1); \
}

#define BLEND_ALPHA50(fg, bg, out)                         \
{                                                          \
   (*(out)) = ((fg + bg + ((fg ^ bg) & BLEND_MASK)) >> 1); \
}

#define BLEND_ALPHA75(fg, bg, out)                                     \
{                                                                      \
   uint16_t color_50 = ((fg + bg + ((fg ^ bg) & BLEND_MASK)) >> 1);    \
   (*(out)) = ((fg + color_50 + ((fg ^ color_50) & BLEND_MASK)) >> 1); \
}

void draw_bmp(int x, int y, const uint16_t *img, int img_width, int img_height)
{
   uint32_t i, j;

   switch (vkb_alpha)
   {
      case VKB_ALPHA_25:
         for (j = 0; j < img_height; j++)
         {
            uint16_t *screen_line    = vkb_video_buffer + ((y + j) * vkb_screen_pitch) + x;
            const uint16_t *img_line = img + j * img_width;
            for (i = 0; i < img_width; i++)
            {
               uint16_t fg = *(img_line++);
               uint16_t bg = *(screen_line);
               BLEND_ALPHA25(fg, bg, screen_line);
               screen_line++;
            }
         }
         break;
      case VKB_ALPHA_50:
         for (j = 0; j < img_height; j++)
         {
            uint16_t *screen_line    = vkb_video_buffer + ((y + j) * vkb_screen_pitch) + x;
            const uint16_t *img_line = img + j * img_width;
            for (i = 0; i < img_width; i++)
            {
               uint16_t fg = *(img_line++);
               uint16_t bg = *(screen_line);
               BLEND_ALPHA50(fg, bg, screen_line);
               screen_line++;
            }
         }
         break;
      case VKB_ALPHA_75:
         for (j = 0; j < img_height; j++)
         {
            uint16_t *screen_line    = vkb_video_buffer + ((y + j) * vkb_screen_pitch) + x;
            const uint16_t *img_line = img + j * img_width;
            for (i = 0; i < img_width; i++)
            {
               uint16_t fg = *(img_line++);
               uint16_t bg = *(screen_line);
               BLEND_ALPHA75(fg, bg, screen_line);
               screen_line++;
            }
         }
         break;
      case VKB_ALPHA_100:
      default:
         for (j = 0; j < img_height; j++)
         {
            uint16_t *screen_line    = vkb_video_buffer + ((y + j) * vkb_screen_pitch) + x;
            const uint16_t *img_line = img + j * img_width;
            for (i = 0; i < img_width; i++)
               *(screen_line++) = *(img_line++);
         }
         break;
   }
}

void draw_box(int x, int y, int width, int height, int thickness, uint16_t color)
{
   uint32_t i, j, k;

   /* Note: For improved visibility, box is drawn
    * with an alpha value one level higher than
    * the keyboard */
   switch (vkb_alpha)
   {
      case VKB_ALPHA_25:
         for (k = 0; k < thickness; k++)
         {
            uint16_t *screen_line_up   = vkb_video_buffer + ((y + k) * vkb_screen_pitch);
            uint16_t *screen_line_down = vkb_video_buffer + ((y + k + height - 1) * vkb_screen_pitch);
            for (i = x; i < x + width + thickness; i++)
            {
               uint16_t bg = *(screen_line_up + i);
               BLEND_ALPHA50(color, bg, screen_line_up + i);
               bg = *(screen_line_down + i);
               BLEND_ALPHA50(color, bg, screen_line_down + i);
            }
            for (j = y + thickness; j < y + height - (thickness >> 1); j++)
            {
               uint32_t offset = (j * vkb_screen_pitch) + x + k;
               uint16_t bg     = *(vkb_video_buffer + offset);
               BLEND_ALPHA50(color, bg, vkb_video_buffer + offset);
               bg = *(vkb_video_buffer + offset + width);
               BLEND_ALPHA50(color, bg, vkb_video_buffer + offset + width);
            }
         }
         break;
      case VKB_ALPHA_50:
         for (k = 0; k < thickness; k++)
         {
            uint16_t *screen_line_up   = vkb_video_buffer + ((y + k) * vkb_screen_pitch);
            uint16_t *screen_line_down = vkb_video_buffer + ((y + k + height - 1) * vkb_screen_pitch);
            for (i = x; i < x + width + thickness; i++)
            {
               uint16_t bg = *(screen_line_up + i);
               BLEND_ALPHA75(color, bg, screen_line_up + i);
               bg = *(screen_line_down + i);
               BLEND_ALPHA75(color, bg, screen_line_down + i);
            }
            for (j = y + thickness; j < y + height - (thickness >> 1); j++)
            {
               uint32_t offset = (j * vkb_screen_pitch) + x + k;
               uint16_t bg     = *(vkb_video_buffer + offset);
               BLEND_ALPHA75(color, bg, vkb_video_buffer + offset);
               bg = *(vkb_video_buffer + offset + width);
               BLEND_ALPHA75(color, bg, vkb_video_buffer + offset + width);
            }
         }
         break;
      case VKB_ALPHA_75:
      case VKB_ALPHA_100:
      default:
         for (k = 0; k < thickness; k++)
         {
            uint16_t *screen_line_up   = vkb_video_buffer + ((y + k) * vkb_screen_pitch);
            uint16_t *screen_line_down = vkb_video_buffer + ((y + k + height - 1) * vkb_screen_pitch);
            for (i = x; i < x + width + thickness; i++)
            {
               *(screen_line_up + i)   = color;
               *(screen_line_down + i) = color;
            }
            for (j = y + thickness; j < y + height - (thickness >> 1); j++)
            {
               uint32_t offset = (j * vkb_screen_pitch) + x + k;
               *(vkb_video_buffer + offset)         = color;
               *(vkb_video_buffer + offset + width) = color;
            }
         }
         break;
   }
}
