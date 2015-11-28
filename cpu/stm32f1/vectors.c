/*
 * Copyright (C) 2014-2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_stm32f1
 * @{
 *
 * @file
 * @brief       Interrupt vector definitions
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 *
 * @}
 */

#include <stdint.h>
#include "vectors_cortexm.h"

/* get the start of the ISR stack as defined in the linkerscript */
extern uint32_t _estack;

/* define a local dummy handler as it needs to be in the same compilation unit
 * as the alias definition */
void dummy_handler(void) {
    dummy_handler_default();
}

/* Cortex-M common interrupt vectors */
WEAK_DEFAULT void isr_svc(void);
WEAK_DEFAULT void isr_pendsv(void);
WEAK_DEFAULT void isr_systick(void);
/* STM32F1 specific interrupt vectors */
WEAK_DEFAULT void isr_wwdg(void);
WEAK_DEFAULT void isr_pvd(void);
WEAK_DEFAULT void isr_tamper(void);
WEAK_DEFAULT void isr_rtc(void);
WEAK_DEFAULT void isr_flash(void);
WEAK_DEFAULT void isr_rcc(void);
WEAK_DEFAULT void isr_exti(void);
WEAK_DEFAULT void isr_dma1_ch1(void);
WEAK_DEFAULT void isr_dma1_ch2(void);
WEAK_DEFAULT void isr_dma1_ch3(void);
WEAK_DEFAULT void isr_dma1_ch4(void);
WEAK_DEFAULT void isr_dma1_ch5(void);
WEAK_DEFAULT void isr_dma1_ch6(void);
WEAK_DEFAULT void isr_dma1_ch7(void);
WEAK_DEFAULT void isr_adc1_2(void);
WEAK_DEFAULT void isr_usb_hp_can1_tx(void);
WEAK_DEFAULT void isr_usb_lp_can1_rx0(void);
WEAK_DEFAULT void isr_can1_rx1(void);
WEAK_DEFAULT void isr_can1_sce(void);
WEAK_DEFAULT void isr_tim1_brk(void);
WEAK_DEFAULT void isr_tim1_up(void);
WEAK_DEFAULT void isr_tim1_trg_com(void);
WEAK_DEFAULT void isr_tim1_cc(void);
WEAK_DEFAULT void isr_tim2(void);
WEAK_DEFAULT void isr_tim3(void);
WEAK_DEFAULT void isr_tim4(void);
WEAK_DEFAULT void isr_i2c1_ev(void);
WEAK_DEFAULT void isr_i2c1_er(void);
WEAK_DEFAULT void isr_i2c2_ev(void);
WEAK_DEFAULT void isr_i2c2_er(void);
WEAK_DEFAULT void isr_spi1(void);
WEAK_DEFAULT void isr_spi2(void);
WEAK_DEFAULT void isr_usart1(void);
WEAK_DEFAULT void isr_usart2(void);
WEAK_DEFAULT void isr_usart3(void);
WEAK_DEFAULT void isr_rtc_alarm(void);
WEAK_DEFAULT void isr_usb_wakeup(void);
WEAK_DEFAULT void isr_tim8_brk(void);
WEAK_DEFAULT void isr_tim8_up(void);
WEAK_DEFAULT void isr_tim8_trg_com(void);
WEAK_DEFAULT void isr_tim8_cc(void);
WEAK_DEFAULT void isr_adc3(void);
WEAK_DEFAULT void isr_fsmc(void);
WEAK_DEFAULT void isr_sdio(void);
WEAK_DEFAULT void isr_tim5(void);
WEAK_DEFAULT void isr_spi3(void);
WEAK_DEFAULT void isr_uart4(void);
WEAK_DEFAULT void isr_uart5(void);
WEAK_DEFAULT void isr_tim6(void);
WEAK_DEFAULT void isr_tim7(void);
WEAK_DEFAULT void isr_dma2_ch1(void);
WEAK_DEFAULT void isr_dma2_ch2(void);
WEAK_DEFAULT void isr_dma2_ch3(void);
WEAK_DEFAULT void isr_dma2_ch4_5(void);

/* interrupt vector table */
ISR_VECTORS const void *interrupt_vector[] = {
    /* Exception stack pointer */
    __extension__ (void*) (&_estack),             /* pointer to the top of the stack */
    /* Cortex-M3 handlers */
    __extension__ (void*) reset_handler_default,  /* entry point of the program */
    __extension__ (void*) nmi_default,            /* non maskable interrupt handler */
    __extension__ (void*) hard_fault_default,     /* hard fault exception */
    __extension__ (void*) mem_manage_default,     /* memory manage exception */
    __extension__ (void*) bus_fault_default,      /* bus fault exception */
    __extension__ (void*) usage_fault_default,    /* usage fault exception */
    __extension__ (void*) (0UL),                  /* Reserved */
    __extension__ (void*) (0UL),                  /* Reserved */
    __extension__ (void*) (0UL),                  /* Reserved */
    __extension__ (void*) (0UL),                  /* Reserved */
    __extension__ (void*) isr_svc,                /* system call interrupt, in RIOT used for
                                     * switching into thread context on boot */
    __extension__ (void*) debug_mon_default,      /* debug monitor exception */
    __extension__ (void*) (0UL),                  /* Reserved */
    __extension__ (void*) isr_pendsv,             /* pendSV interrupt, in RIOT the actual
                                     * context switching is happening here */
    __extension__ (void*) isr_systick,            /* SysTick interrupt, not used in RIOT */
    /* STM specific peripheral handlers */
    __extension__ (void*) isr_wwdg,
    __extension__ (void*) isr_pvd,
    __extension__ (void*) isr_tamper,
    __extension__ (void*) isr_rtc,
    __extension__ (void*) isr_flash,
    __extension__ (void*) isr_rcc,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_dma1_ch1,
    __extension__ (void*) isr_dma1_ch2,
    __extension__ (void*) isr_dma1_ch3,
    __extension__ (void*) isr_dma1_ch4,
    __extension__ (void*) isr_dma1_ch5,
    __extension__ (void*) isr_dma1_ch6,
    __extension__ (void*) isr_dma1_ch7,
    __extension__ (void*) isr_adc1_2,
    __extension__ (void*) isr_usb_hp_can1_tx,
    __extension__ (void*) isr_usb_lp_can1_rx0,
    __extension__ (void*) isr_can1_rx1,
    __extension__ (void*) isr_can1_sce,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_tim1_brk,
    __extension__ (void*) isr_tim1_up,
    __extension__ (void*) isr_tim1_trg_com,
    __extension__ (void*) isr_tim1_cc,
    __extension__ (void*) isr_tim2,
    __extension__ (void*) isr_tim3,
    __extension__ (void*) isr_tim4,
    __extension__ (void*) isr_i2c1_ev,
    __extension__ (void*) isr_i2c1_er,
    __extension__ (void*) isr_i2c2_ev,
    __extension__ (void*) isr_i2c2_er,
    __extension__ (void*) isr_spi1,
    __extension__ (void*) isr_spi2,
    __extension__ (void*) isr_usart1,
    __extension__ (void*) isr_usart2,
    __extension__ (void*) isr_usart3,
    __extension__ (void*) isr_exti,
    __extension__ (void*) isr_rtc_alarm,
    __extension__ (void*) isr_usb_wakeup,
    __extension__ (void*) isr_tim8_brk,
    __extension__ (void*) isr_tim8_up,
    __extension__ (void*) isr_tim8_trg_com,
    __extension__ (void*) isr_tim8_cc,
    __extension__ (void*) isr_adc3,
    __extension__ (void*) isr_fsmc,
    __extension__ (void*) isr_sdio,
    __extension__ (void*) isr_tim5,
    __extension__ (void*) isr_spi3,
    __extension__ (void*) isr_uart4,
    __extension__ (void*) isr_uart5,
    __extension__ (void*) isr_tim6,
    __extension__ (void*) isr_tim7,
    __extension__ (void*) isr_dma2_ch1,
    __extension__ (void*) isr_dma2_ch2,
    __extension__ (void*) isr_dma2_ch3,
    __extension__ (void*) isr_dma2_ch4_5,
};
