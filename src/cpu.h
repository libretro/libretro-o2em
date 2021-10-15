#ifndef CPU_H
#define CPU_H

#include <stdint.h>

extern uint8_t acc;		/* Accumulator */
extern uint16_t pc;		/* Program counter */
extern long clk;		/* clock */

extern uint8_t itimer;		/* Internal timer */
extern uint8_t reg_pnt;	/* pointer to register bank */
extern uint8_t timer_on;  /* 0=timer off/1=timer on */
extern uint8_t count_on;  /* 0=count off/1=count on */

extern uint8_t t_flag;		/* Timer flag */

extern uint8_t psw;		/* Processor status word */
extern uint8_t sp;		/* Stack pointer (part of psw) */

extern uint8_t p1;		/* I/O port 1 */
extern uint8_t p2;		/* I/O port 2 */

extern uint8_t xirq_pend;
extern uint8_t tirq_pend;

void init_cpu(void);
void cpu_exec(void);
void ext_IRQ(void);
void tim_IRQ(void);
void make_psw_debug(void);

extern uint8_t acc;		/* Accumulator */
extern uint16_t pc;		/* Program counter */
extern long clk;		/* clock */

extern uint8_t itimer;	/* Internal timer */
extern uint8_t reg_pnt;	/* pointer to register bank */
extern uint8_t timer_on;  /* 0=timer off/1=timer on */
extern uint8_t count_on;  /* 0=count off/1=count on */
extern uint8_t psw;		/* Processor status word */
extern uint8_t sp;		/* Stack pointer (part of psw) */

extern uint8_t p1;		/* I/O port 1 */
extern uint8_t p2; 		/* I/O port 2 */
extern uint8_t xirq_pend; /* external IRQ pending */
extern uint8_t tirq_pend; /* timer IRQ pending */
extern uint8_t t_flag;	/* Timer flag */

extern uint16_t lastpc;
extern uint16_t A11;		/* PC bit 11 */
extern uint16_t A11ff;
extern uint8_t bs; 		/* Register Bank (part of psw) */
extern uint8_t f0;			/* Flag Bit (part of psw) */
extern uint8_t f1;			/* Flag Bit 1 */
extern uint8_t ac;			/* Aux Carry (part of psw) */
extern uint8_t cy;			/* Carry flag (part of psw) */
extern uint8_t xirq_en;	/* external IRQ's enabled */
extern uint8_t tirq_en;	/* Timer IRQ enabled */
extern uint8_t irq_ex;		/* IRQ executing */

extern int master_count;


#endif  /* CPU_H */
