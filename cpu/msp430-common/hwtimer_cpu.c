/******************************************************************************
Copyright (C) 2013, Freie Universitaet Berlin (FUB). All rights reserved.

These sources were developed at the Freie Universitaet Berlin, Computer Systems
and Telematics group (http://cst.mi.fu-berlin.de).
-------------------------------------------------------------------------------
This file is part of RIOT.

This file subject to the terms and conditions of the GNU Lesser General Public
License. See the file LICENSE in the top level directory for more details.
*******************************************************************************/

#include <stdint.h>

#include <cpu.h>
#include <hwtimer.h>
#include <hwtimer_arch.h>

#define ENABLE_DEBUG (0)
#include <debug.h>


void (*int_handler)(int);
extern void timerA_init(void);
extern void timerB_init(void);

uint16_t overflow_interrupt[ARCH_MAXTIMERS + 1];
uint16_t timer_round;
uint32_t delta;


static void T_disable_interrupt(uint8_t timer)
{
    volatile uint16_t *ptr;
    if (timer >= TIMER_A_COUNT) {
        ptr = &TBCCTL0 + (timer);
    } else {
        ptr = &TA0CCTL0 + (timer);
    }
    *ptr &= ~(CCIFG);
    *ptr &= ~(CCIE);
}

static void T_enable_interrupt(uint8_t timer)
{
    volatile uint16_t *ptr;
    if (timer >= TIMER_A_COUNT) {
        ptr = &TBCCTL0 + (timer);
    } else {
        ptr = &TA0CCTL0 + (timer);
    }
    *ptr &= ~(CCIFG);
    *ptr |= CCIE;
    DEBUG("enabled interrupt for timer %d.\n", timer);
}

static void T_set_nostart(uint16_t value, uint8_t timer)
{
    volatile uint16_t *ptr;
    if (timer >= TIMER_A_COUNT) {
        ptr = &TBCCR0 + (timer);
    } else {
        ptr = &TA0CCR0 + (timer);
    }
    *ptr = value;
}

static void T_set(uint16_t value, uint8_t timer)
{
    DEBUG("Setting timer %u to %u.\n", timer, value);
    T_set_nostart(value, timer);
    T_enable_interrupt(timer);
}

void T_unset(uint8_t timer)
{
    volatile uint16_t *ptr;
    if (timer >= TIMER_A_COUNT) {
        ptr = &TBCCR0 + (timer);
    } else {
        ptr = &TA0CCR0 + (timer);
    }
    T_disable_interrupt(timer);
    *ptr = 0;
}

uint32_t hwtimer_arch_now()
{
    // default counter = timer A counter
    return ((uint32_t) timer_round << 16) + TA0R;
}

void hwtimer_arch_init(void (*handler)(int), uint32_t fcpu)
{
    (void) fcpu;
    timerA_init();
    if (TIMER_B_COUNT > 0) {
        timerB_init();
        delta = ((uint32_t)(TA0R) + 0) - TBR;
    }
    int_handler = handler;
//    T_enable_interrupt(0);
}

void hwtimer_arch_enable_interrupt(void)
{
    for (uint8_t i = 0; i < ARCH_MAXTIMERS; i++) {
        T_enable_interrupt(i);
    }
}

void hwtimer_arch_disable_interrupt(void)
{
    for (uint8_t i = 0; i < ARCH_MAXTIMERS; i++) {
        T_disable_interrupt(i);
    }
}

void hwtimer_arch_set(uint32_t offset, uint8_t timer)
{
    uint32_t value = hwtimer_arch_now() + offset;
    hwtimer_arch_set_absolute(value, timer);
}

void hwtimer_arch_set_absolute(uint32_t value, uint8_t timer)
{
    uint16_t small_value = value % 0xFFFF;
    overflow_interrupt[timer] = (uint16_t)(value >> 16);
    T_set(small_value, timer);
}

void hwtimer_arch_unset(uint8_t timer)
{
    T_unset(timer);
}

