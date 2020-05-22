/* Virtual keyboard layout */

#ifndef __VKEYB_LAYOUT_H
#define __VKEYB_LAYOUT_H

// A key on the virtual keyboard
struct VKey
{
  int scancode;       // scancode of the key
  int x;              // x coordinate of the upper left corner of the key
  int y;              // y coordinate of the upper left corner of the key
  int width;          // width of the key
  int height;         // height of the key
  const struct VKey *left;  // next key at the left side
  const struct VKey *right; // next key at the right side
  const struct VKey *up;    // next key at the up side
  const struct VKey *down;  // next key at the down side
};

// Number of keys
#define ODYSSEY2_KB_KEYS 49

// Keyboard layout
extern const struct VKey o2_kb[];

// Default key
extern const struct VKey *ODYSSEY2_DEFAULT_KEY;

#endif /* __VKEYB_LAYOUT_H */

