/* Virtual keyboard layout */

#include "vkeyb_layout.h"
#include <libretro.h>

const struct VKey *ODYSSEY2_DEFAULT_KEY = &o2_kb[0];

const struct VKey o2_kb[ODYSSEY2_KB_KEYS] =
{
  // 1st row "Numeric" (keys 0 to 9)
  { RETROK_0,   9,  12, 31, 16, &o2_kb[9],  &o2_kb[1],  &o2_kb[39], &o2_kb[10] }, /* 0: 0 */
  { RETROK_1,  42,  12, 31, 16, &o2_kb[0],  &o2_kb[2],  &o2_kb[39], &o2_kb[11] }, /* 1: 1 */
  { RETROK_2,  74,  12, 31, 16, &o2_kb[1],  &o2_kb[3],  &o2_kb[40], &o2_kb[12] }, /* 2: 2 */
  { RETROK_3, 106,  12, 31, 16, &o2_kb[2],  &o2_kb[4],  &o2_kb[48], &o2_kb[13] }, /* 3: 3 */
  { RETROK_4, 139,  12, 31, 16, &o2_kb[3],  &o2_kb[5],  &o2_kb[48], &o2_kb[14] }, /* 4: 4 */
  { RETROK_5, 171,  12, 31, 16, &o2_kb[4],  &o2_kb[6],  &o2_kb[48], &o2_kb[15] }, /* 5: 5 */
  { RETROK_6, 203,  12, 31, 16, &o2_kb[5],  &o2_kb[7],  &o2_kb[48], &o2_kb[16] }, /* 6: 6 */
  { RETROK_7, 235,  12, 31, 16, &o2_kb[6],  &o2_kb[8],  &o2_kb[45], &o2_kb[17] }, /* 7: 7 */
  { RETROK_8, 267,  12, 31, 16, &o2_kb[7],  &o2_kb[9],  &o2_kb[46], &o2_kb[18] }, /* 8: 8 */
  { RETROK_9, 300,  12, 31, 16, &o2_kb[8],  &o2_kb[0],  &o2_kb[47], &o2_kb[19] }, /* 9: 9 */
  // 2nd row "Function/Input/Reset" (keys 10 to 19)
  { RETROK_PLUS,   8,  44, 31, 16, &o2_kb[19], &o2_kb[11], &o2_kb[0],  &o2_kb[20] }, /* 10: + */
  { RETROK_MINUS,  41,  44, 31, 16, &o2_kb[10], &o2_kb[12], &o2_kb[1],  &o2_kb[21] }, /* 11: - */
  { RETROK_ASTERISK,  73,  44, 31, 16, &o2_kb[11], &o2_kb[13], &o2_kb[2],  &o2_kb[22] }, /* 12: * */
  { RETROK_SLASH, 105,  44, 31, 16, &o2_kb[12], &o2_kb[14], &o2_kb[3],  &o2_kb[23] }, /* 13: % */
  { RETROK_EQUALS, 138,  44, 31, 16, &o2_kb[13], &o2_kb[15], &o2_kb[4],  &o2_kb[24] }, /* 14: = */
  { RETROK_LALT, 170,  44, 31, 16, &o2_kb[14], &o2_kb[16], &o2_kb[5],  &o2_kb[25] }, /* 15: YES */
  { RETROK_RALT, 202,  44, 31, 16, &o2_kb[15], &o2_kb[17], &o2_kb[6],  &o2_kb[26] }, /* 16: NO */
  { RETROK_END, 234,  44, 31, 16, &o2_kb[16], &o2_kb[18], &o2_kb[7],  &o2_kb[27] }, /* 17: CLEAR */
  { RETROK_RETURN, 266,  44, 31, 16, &o2_kb[17], &o2_kb[19], &o2_kb[8],  &o2_kb[28] }, /* 18: ENTER */
  { RETROK_UNKNOWN, 299,  44, 31, 16, &o2_kb[18], &o2_kb[10], &o2_kb[9],  &o2_kb[29] }, /* 19: RESET */
  // 3rd row (keys 20 to 29)
  { RETROK_q,   8,  87, 32, 17, &o2_kb[29], &o2_kb[21], &o2_kb[10], &o2_kb[30] }, /* 20: Q */
  { RETROK_w,  41,  87, 32, 17, &o2_kb[20], &o2_kb[22], &o2_kb[11], &o2_kb[31] }, /* 21: W */
  { RETROK_e,  73,  87, 32, 17, &o2_kb[21], &o2_kb[23], &o2_kb[12], &o2_kb[32] }, /* 22: E */
  { RETROK_r, 105,  87, 32, 17, &o2_kb[22], &o2_kb[24], &o2_kb[13], &o2_kb[33] }, /* 23: R */
  { RETROK_t, 137,  87, 32, 17, &o2_kb[23], &o2_kb[25], &o2_kb[14], &o2_kb[34] }, /* 24: T */
  { RETROK_y, 170,  87, 32, 17, &o2_kb[24], &o2_kb[26], &o2_kb[15], &o2_kb[35] }, /* 25: Y */
  { RETROK_u, 202,  87, 32, 17, &o2_kb[25], &o2_kb[27], &o2_kb[16], &o2_kb[36] }, /* 26: U */
  { RETROK_i, 234,  87, 32, 17, &o2_kb[26], &o2_kb[28], &o2_kb[17], &o2_kb[37] }, /* 27: I */
  { RETROK_o, 266,  87, 32, 17, &o2_kb[27], &o2_kb[29], &o2_kb[18], &o2_kb[38] }, /* 28: O */
  { RETROK_p, 299,  87, 32, 17, &o2_kb[28], &o2_kb[20], &o2_kb[19], &o2_kb[38] }, /* 29: P */
  // 4rd row (keys 30 to 38)
  { RETROK_a,  24, 113, 32, 17, &o2_kb[38], &o2_kb[31], &o2_kb[20], &o2_kb[39] }, /* 30: A */
  { RETROK_s,  56, 113, 32, 17, &o2_kb[30], &o2_kb[32], &o2_kb[21], &o2_kb[40] }, /* 31: S */
  { RETROK_d,  88, 113, 32, 17, &o2_kb[31], &o2_kb[33], &o2_kb[22], &o2_kb[41] }, /* 32: D */
  { RETROK_f, 120, 113, 32, 17, &o2_kb[32], &o2_kb[34], &o2_kb[23], &o2_kb[42] }, /* 33: F */
  { RETROK_g, 152, 113, 32, 17, &o2_kb[33], &o2_kb[35], &o2_kb[24], &o2_kb[43] }, /* 34: G */
  { RETROK_h, 185, 113, 32, 17, &o2_kb[34], &o2_kb[36], &o2_kb[25], &o2_kb[44] }, /* 35: H */
  { RETROK_j, 217, 113, 32, 17, &o2_kb[35], &o2_kb[37], &o2_kb[26], &o2_kb[45] }, /* 36: J */
  { RETROK_k, 249, 113, 32, 17, &o2_kb[36], &o2_kb[38], &o2_kb[27], &o2_kb[46] }, /* 37: K */
  { RETROK_l, 282, 113, 32, 17, &o2_kb[37], &o2_kb[30], &o2_kb[28], &o2_kb[47] }, /* 38: L */
  // 5th row (keys 39 to 47)
  { RETROK_z,  40, 139, 32, 18, &o2_kb[47], &o2_kb[40], &o2_kb[30], &o2_kb[1]  }, /* 39: Z */
  { RETROK_x,  72, 139, 32, 18, &o2_kb[39], &o2_kb[41], &o2_kb[31], &o2_kb[2]  }, /* 40: X */
  { RETROK_c, 104, 139, 32, 18, &o2_kb[40], &o2_kb[42], &o2_kb[32], &o2_kb[48] }, /* 41: C */
  { RETROK_v, 136, 139, 32, 18, &o2_kb[41], &o2_kb[43], &o2_kb[33], &o2_kb[48] }, /* 42: V */
  { RETROK_b, 169, 139, 32, 18, &o2_kb[42], &o2_kb[44], &o2_kb[34], &o2_kb[48] }, /* 43: B */
  { RETROK_n, 202, 139, 32, 18, &o2_kb[43], &o2_kb[45], &o2_kb[35], &o2_kb[48] }, /* 44: N */
  { RETROK_m, 234, 139, 32, 18, &o2_kb[44], &o2_kb[46], &o2_kb[36], &o2_kb[7]  }, /* 45: M */
  { RETROK_PERIOD, 266, 139, 32, 18, &o2_kb[45], &o2_kb[47], &o2_kb[37], &o2_kb[8]  }, /* 46: . */
  { RETROK_QUESTION, 299, 139, 32, 18, &o2_kb[46], &o2_kb[39], &o2_kb[38], &o2_kb[9]  }, /* 47: ? */
  // Last row (Space key)
  { RETROK_SPACE, 121, 168, 95, 17, &o2_kb[48], &o2_kb[48], &o2_kb[42], &o2_kb[4]  }, /* 48: Space */
};

