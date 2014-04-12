/*
Copyright 2009, Freie Universitaet Berlin (FUB). All rights reserved.

These sources were developed at the Freie Universitaet Berlin, Computer Systems
and Telematics group (http://cst.mi.fu-berlin.de).
-------------------------------------------------------------------------------
This file is part of FeuerWare.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

FeuerWare is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see http://www.gnu.org/licenses/ .
--------------------------------------------------------------------------------
For further information and questions please use the web site
	http://scatterweb.mi.fu-berlin.de
and the mailinglist (subscription via web site)
	scatterweb@lists.spline.inf.fu-berlin.de
*******************************************************************************/

/**
 * @defgroup    boards_msb430h MSB-430H
 * @ingroup     boards
 * @brief       Support for the ScatterWeb MSB-430H board
 * @{
 *
 * @file        board.h
 * @brief       Basic definitions for the MSB-430H board
 *
 * @author      Freie Universität Berlin, Computer Systems & Telematics, FeuerWhere project
 */

#ifndef _MSB_BOARD_H
#define _MSB_BOARD_H

// for correct inclusion of <msp430.h>
#ifndef __MSP430F1612__
#define __MSP430F1612__
#endif

//MSB430 core
#define MSP430_INITIAL_CPU_SPEED    7372800uL
#define F_CPU                       MSP430_INITIAL_CPU_SPEED
#define F_RC_OSCILLATOR             32768
#define MSP430_HAS_DCOR             1
#define MSP430_HAS_EXTERNAL_CRYSTAL 1

/* LEDs ports MSB430 */
#define LEDS_PxDIR P5DIR
#define LEDS_PxOUT P5OUT
#define LEDS_CONF_RED		0x80
#define LEDS_CONF_GREEN		0x00
#define LEDS_CONF_YELLOW	0x00

#define LED_RED_ON      LEDS_PxOUT &=~LEDS_CONF_RED
#define LED_RED_OFF     LEDS_PxOUT |= LEDS_CONF_RED
#define LED_RED_TOGGLE     LEDS_PxOUT ^= LEDS_CONF_RED

#define LED_GREEN_ON      LEDS_PxOUT &=~LEDS_CONF_GREEN
#define LED_GREEN_OFF     LEDS_PxOUT |= LEDS_CONF_GREEN
#define LED_GREEN_TOGGLE     LEDS_PxOUT ^= LEDS_CONF_GREEN

/* Use P4.1 as debug PIN, since we don't use neither CAN, USART3, nor I2C on
 * this board*/
#define DEBUG_PIN                 (BIT1)

#define BOARD_DEBUG_PIN_OFF       (P4OUT |= DEBUG_PIN)
#define BOARD_DEBUG_PIN_ON        (P4OUT &= ~DEBUG_PIN)
#define BOARD_DEBUG_PIN_TOGGLE    (P4OUT ^= DEBUG_PIN)


#include "board-conf.h"

typedef uint8_t radio_packet_length_t;

/** @} */
#endif // _MSB_BOARD_H
