#include <legacymsp430.h>
#include <board.h>
#include <hwtimer.h>
#include <hwtimer_arch.h>
#include <cpu.h>

#define ENABLE_DEBUG (0)
#include <debug.h>

extern void (*int_handler)(int);

extern void T_unset(short timer);
extern uint16_t overflow_interrupt[ARCH_MAXTIMERS + 1];
extern uint16_t timer_round;

void timerA_init(void)
{
    timer_round = 0;                        // Set to round 0
    TA0CTL = TASSEL_1 + TACLR;               // Clear the timer counter, set ACLK on TimerA
    TA0CTL &= ~TAIFG;                        // Clear the IFG on TimerA
    TA0CTL |= TAIE;                          // Enable the overflow interrupt on TimerA

    volatile unsigned int *ccr = &TACCR0;
    volatile unsigned int *ctl = &TACCTL0;

    for (int i = 0; i < TIMER_A_COUNT; i++) {
        *(ccr + i) = 0;
        *(ctl + i) &= ~(CCIFG);
        *(ctl + i) &= ~(CCIE);
    }

    TACTL |= MC_2;
}

void timerB_init(void)
{
    TBCTL = TBSSEL_1 + TBCLR;               // Clear the timer counter, set ACLK on TimerB
    TBCTL &= ~TBIFG;                        // Clear the IFG on TimerB
    TBCTL &= ~TBIE;                         // Disable the overflow interrupt on TimerB

    volatile unsigned int *ccr = &TBCCR0;
    volatile unsigned int *ctl = &TBCCTL0;

    for (int i = 0; i < TIMER_B_COUNT; i++) {
        *(ccr + i) = 0;
        *(ctl + i) &= ~(CCIFG);
        *(ctl + i) &= ~(CCIE);
    }

    TBCTL |= MC_2;
}

interrupt(TIMERA0_VECTOR) __attribute__((naked)) timera_isr_ccr0(void)
{
    __enter_isr();

    if (overflow_interrupt[0] == timer_round) {
        T_unset(0);
        int_handler(0);
    }

    __exit_isr();

}

interrupt(TIMERA1_VECTOR) __attribute__((naked)) timera_isr_ccr1_6(void)
{
    __enter_isr();

    uint8_t taiv = TAIV;
    uint8_t timer;

    if (taiv & TAIV_TAIFG) {
        timer_round++;
        DEBUG("TimerA overflow (new round).\n");
    } else {
        timer = (taiv >> 1);
        if (overflow_interrupt[timer] == timer_round) {
            T_unset(timer);
            int_handler(timer);
        }
    }

    __exit_isr();
}

interrupt(TIMERB0_VECTOR) __attribute__((naked)) timerb_isr_ccr0(void)
{
    __enter_isr();

    if (overflow_interrupt[TIMER_A_COUNT] == timer_round) {
        T_unset(TIMER_A_COUNT);
        int_handler(TIMER_A_COUNT);
    }

    __exit_isr();

}

interrupt(TIMERB1_VECTOR) __attribute__((naked)) timerb_isr_ccr1_6(void)
{
    __enter_isr();

    uint8_t tbiv = TBIV;
    uint8_t timer;

    if (tbiv & TBIV_TBIFG) {
        DEBUG("TimerB overflow.\n");
    } else {
        timer = (tbiv >> 1) + TIMER_A_COUNT;
        if (overflow_interrupt[timer] == timer_round) {
            T_unset(timer);
            int_handler(timer);
        }
    }

    __exit_isr();
}
