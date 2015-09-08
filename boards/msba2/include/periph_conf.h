/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     boards_msba2
 * @{
 *
 * @file
 * @brief       MSB-A2 peripheral configuration
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef PERIPH_CONF_H_
#define PERIPH_CONF_H_

#include "lpc2387.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Clock configuration
 * @{
 */
#define CLOCK_CORECLOCK     (72000000U)         /* the msba2 runs with 72MHz */

#define CLOCK_PCLK          (CLOCK_CORECLOCK)
/** @} */

/**
 * @brief   Timer configuration, select a number from 1 to 4
 * @{
 */
#define TIMER_NUMOF         (1U)
/** @} */

/**
 * @brief PWM device and pinout configuration
 */
#define PWM_NUMOF           (1)
#define PWM_0_EN            (1)

/* PWM_0 device configuration */
#define PWM_0_CHANNELS      (3)
#define PWM_0_CH0           (3)
#define PWM_0_CH0_MR        PWM1MR3
#define PWM_0_CH1           (4)
#define PWM_0_CH1_MR        PWM1MR4
#define PWM_0_CH2           (5)
#define PWM_0_CH2_MR        PWM1MR5
/* PWM_0 pin configuration */
#define PWM_0_PORT          PINSEL4
#define PWM_0_CH0_PIN       (2)
#define PWM_0_CH1_PIN       (3)
#define PWM_0_CH2_PIN       (4)
#define PWM_0_FUNC          (1)

/**
 * @brief Real Time Clock configuration
 */
#define RTC_NUMOF           (1)

/**
 * @brief uart configuration
 * @{
 */
#define UART_NUMOF          (1)
#define UART_0_EN           (1)
/** @} */

/**
 * @brief SPI configuration
 * @{
 */
#define SPI_NUMOF           (1)
#define SPI_0_EN            (1)

/* SPI 0 device config */
#define SPI_0_POWER             PCSSP0
#define SPI_0_CLK               (PINSEL3 |= BIT8 + BIT9)
#define SPI_0_MISO              (PINSEL3 |= BIT14 + BIT15)
#define SPI_0_MOSI              (PINSEL3 |= BIT16 + BIT17)
#define SPI_0_SSP               SSP0CR0
#define SPI_0_DSS               7
#define SPI_0_TX_EMPTY          (SSP0SR & SSPSR_TFE)
#define SPI_0_BUSY              (SSP0SR & SSPSR_BSY)
#define SPI_0_RX_AVAIL          (SSP0SR & SSPSR_RNE)
#define SPI_0_CCLK              (PCLKSEL1 &= ~(BIT10 | BIT11))
#define SPI_0_CLK_SEL           PCLKSEL1
#define SPI_0_CLK_SHIFT         10
#define SPI_0_CPSR              SSP0CPSR
#define SPI_0_SSP_EN            (SSP0CR1 |= BIT1);
#define SPI_0_DR                SSP0DR
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H_ */
/** @} */
