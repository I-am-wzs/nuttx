/************************************************************************************
 * configs/imxrt1050/include/board.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

#ifndef __CONFIGS_IMXRT1050_EVK_INCLUDE_BOARD_H
#define __CONFIGS_IMXRT1050_EVK_INCLUDE_BOARD_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/
/* Clocking *************************************************************************/

/* Set VDD_SOC to 1.5V */

#define IMXRT_VDD_SOC (0x12)

/*
 * Set Arm PLL (PLL1) to 1200Mhz = (24Mhz * 100) / 2
 * Set Sys PLL (PLL2) to 528Mhz = 1
 *   (0 = 20 * 24Mhz = 480Mhz
 *    1 = 22 * 24Mhz = 528Mhz)
 *
 * Arm clock divider = 2 -> Arm Clock = 600Mhz
 * AHB clock divider = 1
 * IPG clock divider = 4
 *
 */

#define BOARD_XTAL_FREQUENCY    24000000

#define IMXRT_ARM_PLL_SELECT    (100)
#define IMXRT_SYS_PLL_SELECT    (1)
#define IMXRT_ARM_CLOCK_DIVIDER (1)
#define IMXRT_AHB_CLOCK_DIVIDER (0)
#define IMXRT_IPG_CLOCK_DIVIDER (3)

#define BOARD_CPU_FREQUENCY \
  (BOARD_XTAL_FREQUENCY * IMXRT_ARM_PLL_SELECT) / (IMXRT_ARM_CLOCK_DIVIDER + 1)

/* Define lpuart RF and TX pins
 *
 * WARNING: imxrt_pinmux.h should be added and used later.
 */

#define IOMUX_UART              (IOMUX_PULL_UP_100K | IOMUX_CMOS_OUTPUT | \
                                 IOMUX_DRIVE_40OHM | IOMUX_SLEW_FAST | \
                                 IOMUX_SPEED_MEDIUM | IOMUX_SCHMITT_TRIGGER)
#define GPIO_LPUART1_RX_DATA    (GPIO_PERIPH | GPIO_ALT2 | \
                                 GPIO_PADMUX(IMXRT_PADMUX_GPIO_AD_B0_13_INDEX) | \
                                 IOMUX_UART)
#define GPIO_LPUART1_TX_DATA    (GPIO_PERIPH | GPIO_ALT2 | \
                                 GPIO_PADMUX(IMXRT_PADMUX_GPIO_AD_B0_12_INDEX) | \
                                 IOMUX_UART)

/* LED definitions ******************************************************************/
/* There are four LED status indicators located on the EVK Board.  The functions of
 * these LEDs include:
 *
 *   - Main Power Supply(D3)
 *     Green: DC 5V main supply is normal.
 *     Red:   J2 input voltage is over 5.6V.
 *     Off:   The board is not powered.
 *   - Reset RED LED(D15)
 *   - OpenSDA LED(D16)
 *   - USER LED(D18)
 *
 * Only a single LED, D18, is under software control.
 */

/* LED index values for use with board_userled() */

#define BOARD_USERLED     0
#define BOARD_NLEDS       1

/* LED bits for use with board_userled_all() */

#define BOARD_USERLED_BIT (1 << BOARD_USERLED)

/* This LED is not used by the board port unless CONFIG_ARCH_LEDS is
 * defined.  In that case, the usage by the board port is defined in
 * include/board.h and src/sam_autoleds.c. The LED is used to encode
 * OS-related events as follows:
 *
 *   -------------------- ----------------------------- ------
 *   SYMBOL                   Meaning                   LED
 *   -------------------- ----------------------------- ------ */

#define LED_STARTED       0  /* NuttX has been started  OFF    */
#define LED_HEAPALLOCATE  0  /* Heap has been allocated OFF    */
#define LED_IRQSENABLED   0  /* Interrupts enabled      OFF    */
#define LED_STACKCREATED  1  /* Idle stack created      ON     */
#define LED_INIRQ         2  /* In an interrupt         N/C    */
#define LED_SIGNAL        2  /* In a signal handler     N/C    */
#define LED_ASSERTION     2  /* An assertion failed     N/C    */
#define LED_PANIC         3  /* The system has crashed  FLASH  */
#undef  LED_IDLE             /* Not used                       */

/* Thus if the LED is statically on, NuttX has successfully  booted and is,
 * apparently, running normally.  If the LED is flashing at approximately
 * 2Hz, then a fatal error has been detected and the system has halted.
 */

/* Button definitions ***************************************************************/

/* PIO Disambiguation ***************************************************************/

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __CONFIGS_IMXRT1050_EVK_INCLUDE_BOARD_H */
