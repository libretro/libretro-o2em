#ifndef VKBD_DEF_H
#define VKBD_DEF_H 1

#include "libretro.h"

typedef struct {
	char norml[NLETT];
	char shift[NLETT];
	int val;	
} Mvk;

// 0   1   2   3   4   5   6   7   8     9 
//    funtion		input	        reset
// +   -   *   /   =  yes  no clr enter reset
//		alpha
// q   w   e   r   t   y   u   i   o    p
//   a   s   d   f   g   h   j   k   l
//     z   x   c   v   b   n   m   .    ?
//                 space

Mvk MVk[NPLGN*NLIGN*2]={
//PG1
	{ " 0" ," 0"  ,RETROK_0},
	{ " 1" ," 1"  ,RETROK_1 },
	{ " 2" ," 2"  ,RETROK_2 },
	{ " 3" ," 3"  ,RETROK_3 },
	{ " 4" ," 4"  ,RETROK_4 },
	{ " 5" ," 5"  ,RETROK_5 },
	{ " 6" ," 6"  ,RETROK_6 },
	{ " 7" ," 7"  ,RETROK_7 },
	{ " 8" ," 8"  ,RETROK_8 },
	{ " 9" ," 9"  ,RETROK_9},

	{ " +" ," +"  ,RETROK_PLUS},
	{ " -" ," -" , RETROK_MINUS},
	{ " *" ," *" , RETROK_ASTERISK },
	{ " /" ," /"  ,RETROK_SLASH },
	{ " =" ," ="  ,RETROK_EQUALS },
	{ "Ye" ,"Ye"  ,RETROK_PAGEUP },
	{ "No" ,"No"  ,RETROK_PAGEDOWN },
	{ "Cl" ,"Cl"  ,RETROK_END },
	{ "EN" ,"EN"  ,RETROK_RETURN },
	{ "P2" ,"P2"  ,-2},

	{ " q" ," q"  ,RETROK_q},
	{ " w" ," w" , RETROK_w },
	{ " e" ," e" , RETROK_e },
	{ " r" ," r"  ,RETROK_r },
	{ " t" ," t"  ,RETROK_t },
	{ " y" ," y"  ,RETROK_y },
	{ " u" ," u"  ,RETROK_u },
	{ " i" ," i"  ,RETROK_i },
	{ " o" ," o"  ,RETROK_o },
	{ " p" ," p"  ,RETROK_p},

	{ " a" ," a"  ,RETROK_a},
	{ " s" ," s" , RETROK_s },
	{ " d" ," d" , RETROK_d },
	{ " f" ," f"  ,RETROK_f },
	{ " g" ," g"  ,RETROK_g },
	{ " h" ," h"  ,RETROK_h },
	{ " j" ," j"  ,RETROK_j },
	{ " k" ," k"  ,RETROK_k },
	{ " l" ," l"  ,RETROK_l },
	{ "SD" ,"SD" ,-12},

	{ " z" ," z"  ,RETROK_z},
	{ " x" ," x"  ,RETROK_x },
	{ " c" ," c"  ,RETROK_c },
	{ " v" ," v"  ,RETROK_v },
	{ " b" ," b"  ,RETROK_b },
	{ " n" ," n"  ,RETROK_n },
	{ " m" ," m"  ,RETROK_m },
	{ " ." ," ."  ,RETROK_PERIOD },
	{ " ?" ," ?"  ,RETROK_QUESTION },
	{ "Sp" ,"Sp"  ,RETROK_SPACE},
//PG2
	{ " 0" ," 0"  ,RETROK_0},
	{ " 1" ," 1"  ,RETROK_1 },
	{ " 2" ," 2"  ,RETROK_2 },
	{ " 3" ," 3"  ,RETROK_3 },
	{ " 4" ," 4"  ,RETROK_4 },
	{ " 5" ," 5"  ,RETROK_5 },
	{ " 6" ," 6"  ,RETROK_6 },
	{ " 7" ," 7"  ,RETROK_7 },
	{ " 8" ," 8"  ,RETROK_8 },
	{ " 9" ," 9"  ,RETROK_9},

	{ "Qt" ,"Qt"  ,-6},
	{ "Ps" ,"Ps" , -7},
	{ "Rs" ,"Rs" , -8},
	{ "SS" ,"SS"  ,-9},
	{ "Sv" ,"Sv"  ,-10 },
	{ "Lo" ,"Lo"  ,-11 },
	{ "Ij" ,"Ij"  ,-5 },
	{ "Co" ,"Co"  ,-3  },
	{ "Kb" ,"Kb"  ,-4 },
	{ "P1" ,"P1"  ,-2},

	{ " q" ," q"  ,RETROK_q},
	{ " w" ," w" , RETROK_w },
	{ " e" ," e" , RETROK_e },
	{ " r" ," r"  ,RETROK_r },
	{ " t" ," t"  ,RETROK_t },
	{ " y" ," y"  ,RETROK_y },
	{ " u" ," u"  ,RETROK_u },
	{ " i" ," i"  ,RETROK_i },
	{ " o" ," o"  ,RETROK_o },
	{ " p" ," p"  ,RETROK_p},

	{ " a" ," a"  ,RETROK_a},
	{ " s" ," s" , RETROK_s },
	{ " d" ," d" , RETROK_d },
	{ " f" ," f"  ,RETROK_f },
	{ " g" ," g"  ,RETROK_g },
	{ " h" ," h"  ,RETROK_h },
	{ " j" ," j"  ,RETROK_j },
	{ " k" ," k"  ,RETROK_k },
	{ " l" ," l"  ,RETROK_l },
	{ "SD" ,"SD" ,-12},

	{ " z" ," z"  ,RETROK_z},
	{ " x" ," x"  ,RETROK_x },
	{ " c" ," c"  ,RETROK_c },
	{ " v" ," v"  ,RETROK_v },
	{ " b" ," b"  ,RETROK_b },
	{ " n" ," n"  ,RETROK_n },
	{ " m" ," m"  ,RETROK_m },
	{ " ." ," ."  ,RETROK_PERIOD },
	{ " ?" ," ?"  ,RETROK_QUESTION },
	{ "Sp" ,"Sp"  ,RETROK_SPACE},

} ;

#endif
