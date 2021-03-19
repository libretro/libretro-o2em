/* Virtual keyboard */

#ifndef __VKEYB_H
#define __VKEYB_H

#include <stdint.h>
#include "../../libretro-common/include/boolean.h"

// Directions to move the currently selected key
enum VkbMoveDirection { VKB_MOVE_LEFT, VKB_MOVE_RIGHT, VKB_MOVE_UP, VKB_MOVE_DOWN };
// Positions of the virtual keyboard
enum VkbPosition { VKB_POS_DOWN, VKB_POS_UP };

// Opacity of the virtual keyboard
enum VkbAlpha
{
	VKB_ALPHA_25 = 0,
	VKB_ALPHA_50,
	VKB_ALPHA_75,
	VKB_ALPHA_100
};

// Configure the virtual keyboard feature
extern void vkb_configure_virtual_keyboard(uint16_t *video_buffer, int width, int height, int pitch);
// Set the virtual keyboard transparency
extern void vkb_set_virtual_keyboard_transparency(enum VkbAlpha alpha);
// Set the position of the virtual keyboard
extern void vkb_set_virtual_keyboard_position(enum VkbPosition position);
// Get the currently configured position of the virtual keyboard
extern enum VkbPosition vkb_get_virtual_keyboard_position(void);

// Show the virtual keyboard during the current video frame
extern void vkb_show_virtual_keyboard(void);

// Move the selected key in the given direction
extern void vkb_move_key(enum VkbMoveDirection direction);
// Return the scancode of the currently selected key
extern int vkb_get_current_key_scancode(void);
// Move to the key at the x/y screen coordinates.
// Returns false if no key was found at these coordinates.
extern bool vkb_move_at(int x, int y);

#endif /* __VKEYB_H */
