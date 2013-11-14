/******************************************************************************
Copyright (C) 2013, Freie Universitaet Berlin (FUB). All rights reserved.

These sources were developed at the Freie Universitaet Berlin, Computer Systems
and Telematics group (http://cst.mi.fu-berlin.de).
-------------------------------------------------------------------------------
This file is part of RIOT.

This file subject to the terms and conditions of the GNU Lesser General Public
License. See the file LICENSE in the top level directory for more details.
*******************************************************************************/

#ifndef __HWTIMER_CPU_H
#define __HWTIMER_CPU_H

#ifdef CC430
#include <cc430f6137.h>
#else
#include <msp430x16x.h>
#endif
#include <stdint.h>

#if defined (__MSP430_HAS_TA2__)
#define TIMER_A_COUNT 2
#elif defined (__MSP430_HAS_TA3__)
#define TIMER_A_COUNT 3
#elif defined (__MSP430_HAS_T0A5__)
#define TIMER_A_COUNT 5
#else
#define TIMER_A_COUNT 0
#endif

#if defined (__MSP430_HAS_TB3__)
#define TIMER_B_COUNT 3
#elif defined (__MSP430_HAS_TB5__)
#define TIMER_B_COUNT 5
#elif defined (__MSP430_HAS_TB7__)
#define TIMER_B_COUNT 7
#else
#define TIMER_B_COUNT 0
#endif

#define ARCH_MAXTIMERS (TIMER_A_COUNT + TIMER_B_COUNT)

#ifndef ARCH_MAXTIMERS
#warning "ARCH_MAXTIMERS UNSET!"
#define ARCH_MAXTIMERS 0
#endif

#define HWTIMER_SPEED 32768
#define HWTIMER_MAXTICKS (0xFFFFFFFF)

#endif // __HWTIMER_CPU_H
