#include <debug.h>
#include <msp430x16x.h>
#include <hwtimer.h>
#include <hwtimer_arch.h>
#include <cpu.h>

extern void (*int_handler)(int);

extern void T_unset(short timer);
extern uint16_t overflow_interrupt[ARCH_MAXTIMERS + 1];
extern uint16_t timer_round;

void timerA_init(void)
{
    volatile unsigned int *ccr;
    volatile unsigned int *ctl;
    timer_round = 0;                        // Set to round 0
    TACTL = TASSEL_1 + TACLR;               // Clear the timer counter, set ACLK on TimerA
    TACTL &= ~TAIFG;                        // Clear the IFG on TimerA
    TACTL |= TAIE;                          // Enable the overflow interrupt on TimerA

    for (int i = 0; i < TIMER_A_COUNT; i++) {
        ccr = &TACCR0 + (i);
        ctl = &TACCTL0 + (i);
        *ccr = 0;
        *ctl &= ~(CCIFG);
        *ctl &= ~(CCIE);
    }

    TACTL |= MC_2;
}

void timerB_init(void)
{
    volatile unsigned int *ccr;
    volatile unsigned int *ctl;
    TBCTL = TBSSEL_1 + TBCLR;               // Clear the timer counter, set ACLK on TimerB
    TBCTL &= ~TBIFG;                        // Clear the IFG on TimerB
    TBCTL &= ~TBIE;                         // Disable the overflow interrupt on TimerB

    for (int i = 0; i < TIMER_B_COUNT; i++) {
        ccr = &TBCCR0 + (i);
        ctl = &TBCCTL0 + (i);
        *ccr = 0;
        *ctl &= ~(CCIFG);
        *ctl &= ~(CCIE);
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

    if (taiv & TAIV_TAIFG) {
        timer_round++;
        DEBUG("TimerA overflow (new round).\n");
    } else {
        uint8_t timer = (taiv >> 1);
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

    if (tbiv & TBIV_TBIFG) {
        DEBUG("TimerB overflow.\n");
    } else {
        uint8_t timer = (tbiv >> 1) + TIMER_A_COUNT;
        if (overflow_interrupt[timer] == timer_round) {
            T_unset(timer);
            int_handler(timer);
        }
    }

    __exit_isr();
}
