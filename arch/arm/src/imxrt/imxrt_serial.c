/****************************************************************************
 * arch/arm/src/imxrt/imxrt_serial.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Ivan Ucherdzhiev <ivanucherdjiev@gmail.com>
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/init.h>
#include <nuttx/serial/serial.h>
#include <arch/serial.h>
#include <arch/board/board.h>

#include "chip.h"
#include "up_arch.h"
#include "up_internal.h"

#include "chip/imxrt_lpuart.h"
#include "imxrt_config.h"
#include "imxrt_lowputc.h"

#ifdef USE_SERIALDRIVER

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Which LPUART with be tty0/console and which tty1-7?  The console will always
 * be ttyS0.  If there is no console then will use the lowest numbered UART.
 */

/* First pick the console and ttys0.  This could be any of LPUART1-8 */

#if defined(CONFIG_LPUART1_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart1port /* LPUART1 is console */
#    define TTYS0_DEV           g_uart1port /* LPUART1 is ttyS0 */
#    define UART1_ASSIGNED      1
#elif defined(CONFIG_LPUART2_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart2port /* LPUART2 is console */
#    define TTYS0_DEV           g_uart2port /* LPUART2 is ttyS0 */
#    define UART2_ASSIGNED      1
#elif defined(CONFIG_LPUART3_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart3port /* LPUART3 is console */
#    define TTYS0_DEV           g_uart3port /* LPUART3 is ttyS0 */
#    define UART3_ASSIGNED      1
#elif defined(CONFIG_LPUART4_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart4port /* LPUART4 is console */
#    define TTYS0_DEV           g_uart4port /* LPUART4 is ttyS0 */
#    define UART4_ASSIGNED      1
#elif defined(CONFIG_LPUART5_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart5port /* LPUART5 is console */
#    define TTYS5_DEV           g_uart5port /* LPUART5 is ttyS0 */
#    define UART5_ASSIGNED      1
#elif defined(CONFIG_LPUART6_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart6port /* LPUART6 is console */
#    define TTYS6_DEV           g_uart6port /* LPUART6 is ttyS0 */
#    define UART6_ASSIGNED      1
#elif defined(CONFIG_LPUART7_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart7port /* LPUART7 is console */
#    define TTYS7_DEV           g_uart7port /* LPUART7 is ttyS0 */
#    define UART7_ASSIGNED      1
#elif defined(CONFIG_LPUART8_SERIAL_CONSOLE)
#    define CONSOLE_DEV         g_uart8port /* LPUART8 is console */
#    define TTYS8_DEV           g_uart8port /* LPUART8 is ttyS0 */
#    define UART8_ASSIGNED      1
#else
#  undef CONSOLE_DEV                        /* No console */
#  if defined(CONFIG_IMXRT_LPUART1)
#    define TTYS0_DEV           g_uart1port /* LPUART1 is ttyS0 */
#    define UART1_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART2)
#    define TTYS0_DEV           g_uart2port /* LPUART2 is ttyS0 */
#    define UART2_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART3)
#    define TTYS0_DEV           g_uart3port /* LPUART3 is ttyS0 */
#    define UART3_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART4)
#    define TTYS0_DEV           g_uart4port /* LPUART4 is ttyS0 */
#    define UART4_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART5)
#    define TTYS0_DEV           g_uart5port /* LPUART5 is ttyS0 */
#    define UART5_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART6)
#    define TTYS0_DEV           g_uart6port /* LPUART6 is ttyS0 */
#    define UART6_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART7)
#    define TTYS0_DEV           g_uart7port /* LPUART7 is ttyS0 */
#    define UART7_ASSIGNED      1
#  elif defined(CONFIG_IMXRT_LPUART8)
#    define TTYS0_DEV           g_uart8port /* LPUART8 is ttyS0 */
#    define UART8_ASSIGNED      1
#  endif
#endif

/* Pick ttys1.  This could be any of UART1-8 excluding the console UART.
 * One of UART1-8 could be the console; one of UART1-8 has already been
 * assigned to ttys0.
 */

#if defined(CONFIG_IMXRT_LPUART1) && !defined(UART1_ASSIGNED)
#  define TTYS1_DEV           g_uart1port /* LPUART1 is ttyS1 */
#  define UART1_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART2) && !defined(UART2_ASSIGNED)
#  define TTYS1_DEV           g_uart2port /* LPUART2 is ttyS1 */
#  define UART2_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART3) && !defined(UART3_ASSIGNED)
#  define TTYS1_DEV           g_uart3port /* LPUART3 is ttyS1 */
#  define UART3_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART4) && !defined(UART4_ASSIGNED)
#  define TTYS1_DEV           g_uart4port /* LPUART4 is ttyS1 */
#  define UART4_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART5) && !defined(UART5_ASSIGNED)
#  define TTYS1_DEV           g_uart5port /* LPUART5 is ttyS1 */
#  define UART5_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART6) && !defined(UART6_ASSIGNED)
#  define TTYS1_DEV           g_uart6port /* LPUART6 is ttyS1 */
#  define UART6_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS1_DEV           g_uart7port /* LPUART7 is ttyS1 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS1_DEV           g_uart8port /* LPUART8 is ttyS1 */
#  define UART8_ASSIGNED      1
#endif

/* Pick ttys2.  This could be one of UART2-8. It can't be UART1 because that
 * was either assigned as ttyS0 or ttys1.  One of UART 1-8 could be the
 * console.  One of UART2-8 has already been assigned to ttys0 or ttyS1.
 */

#if defined(CONFIG_IMXRT_LPUART2) && !defined(UART2_ASSIGNED)
#  define TTYS2_DEV           g_uart2port /* LPUART2 is ttyS2 */
#  define UART2_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART3) && !defined(UART3_ASSIGNED)
#  define TTYS2_DEV           g_uart3port /* LPUART3 is ttyS2 */
#  define UART3_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART4) && !defined(UART4_ASSIGNED)
#  define TTYS2_DEV           g_uart4port /* LPUART4 is ttyS2 */
#  define UART4_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART5) && !defined(UART5_ASSIGNED)
#  define TTYS2_DEV           g_uart5port /* LPUART5 is ttyS2 */
#  define UART5_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART6) && !defined(UART6_ASSIGNED)
#  define TTYS2_DEV           g_uart6port /* LPUART6 is ttyS2 */
#  define UART6_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS2_DEV           g_uart7port /* LPUART7 is ttyS2 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS2_DEV           g_uart8port /* LPUART8 is ttyS2 */
#  define UART8_ASSIGNED      1
#endif

/* Pick ttys3. This could be one of UART3-8. It can't be UART1-2 because
 * those have already been assigned to ttsyS0, 1, or 2.  One of
 * UART3-8 could also be the console.  One of UART3-8 has already
 * been assigned to ttys0, 1, or 3.
 */

#if defined(CONFIG_IMXRT_LPUART3) && !defined(UART3_ASSIGNED)
#  define TTYS3_DEV           g_uart3port /* LPUART3 is ttyS3 */
#  define UART3_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART4) && !defined(UART4_ASSIGNED)
#  define TTYS3_DEV           g_uart4port /* LPUART4 is ttyS3 */
#  define UART4_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART5) && !defined(UART5_ASSIGNED)
#  define TTYS3_DEV           g_uart5port /* LPUART5 is ttyS3 */
#  define UART5_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART6) && !defined(UART6_ASSIGNED)
#  define TTYS3_DEV           g_uart6port /* LPUART6 is ttyS3 */
#  define UART6_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS3_DEV           g_uart7port /* LPUART7 is ttyS3 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS3_DEV           g_uart8port /* LPUART8 is ttyS3 */
#  define UART8_ASSIGNED      1
#endif

/* Pick ttys4. This could be one of UART4-8. It can't be UART1-3 because
 * those have already been assigned to ttsyS0, 1, 2 or 3.  One of
 * UART 4-8 could be the console.  One of UART4-8 has already been
 * assigned to ttys0, 1, 3, or 4.
 */

#if defined(CONFIG_IMXRT_LPUART4) && !defined(UART4_ASSIGNED)
#  define TTYS4_DEV           g_uart4port /* LPUART4 is ttyS4 */
#  define UART4_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART5) && !defined(UART5_ASSIGNED)
#  define TTYS4_DEV           g_uart5port /* LPUART5 is ttyS4 */
#  define UART5_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART6) && !defined(UART6_ASSIGNED)
#  define TTYS4_DEV           g_uart6port /* LPUART6 is ttyS4 */
#  define UART6_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS4_DEV           g_uart7port /* LPUART7 is ttyS4 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS4_DEV           g_uart8port /* LPUART8 is ttyS4 */
#  define UART8_ASSIGNED      1
#endif

/* Pick ttys5. This could be one of UART5-8. It can't be UART1-4 because
 * those have already been assigned to ttsyS0, 1, 2, 3 or 4.  One of
 * UART 5-8 could be the console.  One of UART5-8 has already been
 * assigned to ttys0, 1, 2, 3, or 4.
 */
#if defined(CONFIG_IMXRT_LPUART5) && !defined(UART5_ASSIGNED)
#  define TTYS5_DEV           g_uart5port /* LPUART5 is ttyS5 */
#  define UART5_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART6) && !defined(UART6_ASSIGNED)
#  define TTYS5_DEV           g_uart6port /* LPUART6 is ttyS5 */
#  define UART6_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS5_DEV           g_uart7port /* LPUART7 is ttyS5 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS5_DEV           g_uart8port /* LPUART8 is ttyS5 */
#  define UART8_ASSIGNED      1
#endif

/* Pick ttys6. This could be one of UART6-8. It can't be UART1-5 because
 * those have already been assigned to ttsyS0, 1, 2, 3, 4 or 5.  One of
 * UART 6-8 could be the console.  One of UART6-8 has already been
 * assigned to ttys0, 1, 2, 3, 4 or 5.
 */

#if defined(CONFIG_IMXRT_LPUART6) && !defined(UART6_ASSIGNED)
#  define TTYS6_DEV           g_uart6port /* LPUART6 is ttyS5 */
#  define UART6_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS6_DEV           g_uart7port /* LPUART7 is ttyS5 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS6_DEV           g_uart8port /* LPUART8 is ttyS5 */
#  define UART8_ASSIGNED      1
#endif

/* Pick ttys7. This could be one of UART7-8. It can't be UART1-6 because
 * those have already been assigned to ttsyS0, 1, 2, 3, 4, 5 or 6.  One of
 * UART 7-8 could be the console.  One of UART7-8 has already been
 * assigned to ttys0, 1, 2, 3, 4, 5 or 6.
 */

#if defined(CONFIG_IMXRT_LPUART7) && !defined(UART7_ASSIGNED)
#  define TTYS7_DEV           g_uart7port /* LPUART7 is ttyS5 */
#  define UART7_ASSIGNED      1
#elif defined(CONFIG_IMXRT_LPUART8) && !defined(UART8_ASSIGNED)
#  define TTYS7_DEV           g_uart8port /* LPUART8 is ttyS5 */
#  define UART8_ASSIGNED      1
#endif

/* UART, if available, should have been assigned to ttyS0-7. */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct imxrt_uart_s
{
  uint32_t uartbase;    /* Base address of UART registers */
  uint32_t baud;        /* Configured baud */
  uint32_t ie;          /* Saved enabled interrupts */
  uint8_t  irq;         /* IRQ associated with this UART */
  uint8_t  parity;      /* 0=none, 1=odd, 2=even */
  uint8_t  bits;        /* Number of bits (7 or 8) */
  uint8_t  stopbits2:1; /* 1: Configure with 2 stop bits vs 1 */
  uint8_t  hwfc:1;      /* 1: Hardware flow control */
  uint8_t  reserved:6;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static inline uint32_t imxrt_serialin(struct imxrt_uart_s *priv,
              uint32_t offset);
static inline void imxrt_serialout(struct imxrt_uart_s *priv, uint32_t offset,
              uint32_t value);
static inline void imxrt_disableuartint(struct imxrt_uart_s *priv,
              uint32_t *ie);
static inline void imxrt_restoreuartint(struct imxrt_uart_s *priv,
              uint32_t ie);

static int  imxrt_setup(struct uart_dev_s *dev);
static void imxrt_shutdown(struct uart_dev_s *dev);
static int  imxrt_attach(struct uart_dev_s *dev);
static void imxrt_detach(struct uart_dev_s *dev);
static int  imxrt_interrupt(int irq, void *context, FAR void *arg);
static int  imxrt_ioctl(struct file *filep, int cmd, unsigned long arg);
static int  imxrt_receive(struct uart_dev_s *dev, uint32_t *status);
static void imxrt_rxint(struct uart_dev_s *dev, bool enable);
static bool imxrt_rxavailable(struct uart_dev_s *dev);
static void imxrt_send(struct uart_dev_s *dev, int ch);
static void imxrt_txint(struct uart_dev_s *dev, bool enable);
static bool imxrt_txready(struct uart_dev_s *dev);
static bool imxrt_txempty(struct uart_dev_s *dev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Used to assure mutually exclusive access up_putc() */
/* REVISIT:  This is referenced anywhere and generates a warning. */

static sem_t g_putc_lock = SEM_INITIALIZER(1);

/* Serial driver UART operations */

static const struct uart_ops_s g_uart_ops =
{
  .setup          = imxrt_setup,
  .shutdown       = imxrt_shutdown,
  .attach         = imxrt_attach,
  .detach         = imxrt_detach,
  .ioctl          = imxrt_ioctl,
  .receive        = imxrt_receive,
  .rxint          = imxrt_rxint,
  .rxavailable    = imxrt_rxavailable,
#ifdef CONFIG_SERIAL_IFLOWCONTROL
  .rxflowcontrol  = NULL,
#endif
  .send           = imxrt_send,
  .txint          = imxrt_txint,
  .txready        = imxrt_txready,
  .txempty        = imxrt_txempty,
};

/* I/O buffers */

#ifdef CONFIG_IMXRT_LPUART1
static char g_uart1rxbuffer[CONFIG_LPUART1_RXBUFSIZE];
static char g_uart1txbuffer[CONFIG_LPUART1_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART2
static char g_uart2rxbuffer[CONFIG_LPUART2_RXBUFSIZE];
static char g_uart2txbuffer[CONFIG_LPUART2_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART3
static char g_uart3rxbuffer[CONFIG_LPUART3_RXBUFSIZE];
static char g_uart3txbuffer[CONFIG_LPUART3_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART4
static char g_uart4rxbuffer[CONFIG_LPUART4_RXBUFSIZE];
static char g_uart4txbuffer[CONFIG_LPUART4_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART5
static char g_uart5rxbuffer[CONFIG_LPUART5_RXBUFSIZE];
static char g_uart5txbuffer[CONFIG_LPUART5_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART6
static char g_uart6rxbuffer[CONFIG_LPUART6_RXBUFSIZE];
static char g_uart6txbuffer[CONFIG_LPUART6_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART7
static char g_uart7rxbuffer[CONFIG_LPUART7_RXBUFSIZE];
static char g_uart7txbuffer[CONFIG_LPUART7_TXBUFSIZE];
#endif

#ifdef CONFIG_IMXRT_LPUART8
static char g_uart8rxbuffer[CONFIG_LPUART8_RXBUFSIZE];
static char g_uart8txbuffer[CONFIG_LPUART8_TXBUFSIZE];
#endif

/* This describes the state of the IMXRT lpuart1 port. */

#ifdef CONFIG_IMXRT_LPUART1
static struct imxrt_uart_s g_uart1priv =
{
  .uartbase       = IMXRT_LPUART1_BASE,
  .baud           = CONFIG_LPUART1_BAUD,
  .irq            = IMXRT_IRQ_LPUART1,
  .parity         = CONFIG_LPUART1_PARITY,
  .bits           = CONFIG_LPUART1_BITS,
  .stopbits2      = CONFIG_LPUART1_2STOP,
};

static struct uart_dev_s g_uart1port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART1_RXBUFSIZE,
    .buffer = g_uart1rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART1_TXBUFSIZE,
    .buffer = g_uart1txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart1priv,
};
#endif

/* This describes the state of the IMXRT lpuart2 port. */

#ifdef CONFIG_IMXRT_LPUART2
static struct imxrt_uart_s g_uart2priv =
{
  .uartbase       = IMXRT_LPUART2_BASE,
  .baud           = CONFIG_LPUART2_BAUD,
  .irq            = IMXRT_IRQ_LPUART2,
  .parity         = CONFIG_LPUART2_PARITY,
  .bits           = CONFIG_LPUART2_BITS,
  .stopbits2      = CONFIG_LPUART2_2STOP,
};

static struct uart_dev_s g_uart2port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART2_RXBUFSIZE,
    .buffer = g_uart2rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART2_TXBUFSIZE,
    .buffer = g_uart2txbuffer,
   },
  .ops      = &g_uart_ops,
  .priv     = &g_uart2priv,
};
#endif

#ifdef CONFIG_IMXRT_LPUART3
static struct imxrt_uart_s g_uart3priv =
{
  .uartbase       = IMXRT_LPUART3_BASE,
  .baud           = CONFIG_LPUART3_BAUD,
  .irq            = IMXRT_IRQ_LPUART3,
  .parity         = CONFIG_LPUART3_PARITY,
  .bits           = CONFIG_LPUART3_BITS,
  .stopbits2      = CONFIG_LPUART3_2STOP,
};

static struct uart_dev_s g_uart3port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART3_RXBUFSIZE,
    .buffer = g_uart3rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART3_TXBUFSIZE,
    .buffer = g_uart3txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart3priv,
};
#endif

#ifdef CONFIG_IMXRT_LPUART4
static struct imxrt_uart_s g_uart4priv =
{
  .uartbase       = IMXRT_LPUART4_BASE,
  .baud           = CONFIG_LPUART4_BAUD,
  .irq            = IMXRT_IRQ_LPUART4,
  .parity         = CONFIG_LPUART4_PARITY,
  .bits           = CONFIG_LPUART4_BITS,
  .stopbits2      = CONFIG_LPUART4_2STOP,
};

static struct uart_dev_s g_uart4port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART4_RXBUFSIZE,
    .buffer = g_uart4rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART4_TXBUFSIZE,
    .buffer = g_uart4txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart4priv,
};
#endif

#ifdef CONFIG_IMXRT_LPUART5
static struct imxrt_uart_s g_uart5priv =
{
  .uartbase       = IMXRT_LPUART5_BASE,
  .baud           = CONFIG_LPUART5_BAUD,
  .irq            = IMXRT_IRQ_LPUART5,
  .parity         = CONFIG_LPUART5_PARITY,
  .bits           = CONFIG_LPUART5_BITS,
  .stopbits2      = CONFIG_LPUART5_2STOP,
};

static struct uart_dev_s g_uart5port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART5_RXBUFSIZE,
    .buffer = g_uart5rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART5_TXBUFSIZE,
    .buffer = g_uart5txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart5priv,
};
#endif

#ifdef CONFIG_IMXRT_LPUART6
static struct imxrt_uart_s g_uart6priv =
{
  .uartbase       = IMXRT_LPUART6_BASE,
  .baud           = CONFIG_LPUART6_BAUD,
  .irq            = IMXRT_IRQ_LPUART6,
  .parity         = CONFIG_LPUART6_PARITY,
  .bits           = CONFIG_LPUART6_BITS,
  .stopbits2      = CONFIG_LPUART6_2STOP,
};

static struct uart_dev_s g_uart6port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART6_RXBUFSIZE,
    .buffer = g_uart6rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART6_TXBUFSIZE,
    .buffer = g_uart6txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart6priv,
};
#endif

#ifdef CONFIG_IMXRT_LPUART7
static struct imxrt_uart_s g_uart7priv =
{
  .uartbase       = IMXRT_LPUART7_BASE,
  .baud           = CONFIG_LPUART7_BAUD,
  .irq            = IMXRT_IRQ_LPUART7,
  .parity         = CONFIG_LPUART7_PARITY,
  .bits           = CONFIG_LPUART7_BITS,
  .stopbits2      = CONFIG_LPUART7_2STOP,
};

static struct uart_dev_s g_uart7port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART7_RXBUFSIZE,
    .buffer = g_uart7rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART7_TXBUFSIZE,
    .buffer = g_uart7txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart7priv,
};
#endif

#ifdef CONFIG_IMXRT_LPUART8
static struct imxrt_uart_s g_uart8priv =
{
  .uartbase       = IMXRT_LPUART8_BASE,
  .baud           = CONFIG_LPUART8_BAUD,
  .irq            = IMXRT_IRQ_LPUART8,
  .parity         = CONFIG_LPUART8_PARITY,
  .bits           = CONFIG_LPUART8_BITS,
  .stopbits2      = CONFIG_LPUART8_2STOP,
};

static struct uart_dev_s g_uart8port =
{
  .recv     =
  {
    .size   = CONFIG_LPUART8_RXBUFSIZE,
    .buffer = g_uart8rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_LPUART8_TXBUFSIZE,
    .buffer = g_uart8txbuffer,
  },
  .ops      = &g_uart_ops,
  .priv     = &g_uart8priv,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: imxrt_serialin
 ****************************************************************************/

static inline uint32_t imxrt_serialin(struct imxrt_uart_s *priv,
                                      uint32_t offset)
{
  return getreg32(priv->uartbase + offset);
}

/****************************************************************************
 * Name: imxrt_serialout
 ****************************************************************************/

static inline void imxrt_serialout(struct imxrt_uart_s *priv, uint32_t offset,
                                   uint32_t value)
{
  putreg32(value, priv->uartbase + offset);
}

/****************************************************************************
 * Name: imxrt_disableuartint
 ****************************************************************************/

static inline void imxrt_disableuartint(struct imxrt_uart_s *priv,
                                        uint32_t *ie)
{
  irqstate_t flags;
  uint32_t regval = 0;

  flags  = enter_critical_section();
  regval = imxrt_serialin(priv, IMXRT_LPUART_CTRL_OFFSET);

  /* Return the current Rx and Tx interrupt state */

  if (ie != NULL)
    {
      *ie = regval & LPUART_ALL_INTS;
    }

  regval &= ~ LPUART_ALL_INTS;
  imxrt_serialout(priv, IMXRT_LPUART_CTRL_OFFSET, regval);
  leave_critical_section(flags);
}

/****************************************************************************
 * Name: imxrt_restoreuartint
 ****************************************************************************/

static inline void imxrt_restoreuartint(struct imxrt_uart_s *priv,
                                        uint32_t ie)
{
  /* Enable/disable any interrupts that are currently disabled but should be
   * enabled/disabled.
   */

  uint32_t regval = 0;

  regval = imxrt_serialin(priv, IMXRT_LPUART_CTRL_OFFSET) | ie;
  imxrt_serialout(priv, IMXRT_LPUART_CTRL_OFFSET, regval);
}

/****************************************************************************
 * Name: imxrt_setup
 *
 * Description:
 *   Configure the UART baud, bits, parity, fifos, etc. This
 *   method is called the first time that the serial port is
 *   opened.
 *
 ****************************************************************************/

static int imxrt_setup(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;
#ifndef CONFIG_SUPPRESS_LPUART_CONFIG
  struct uart_config_s config;
  int ret;

  /* Configure the UART */

  config.baud       = priv->baud;       /* Configured baud */
  config.parity     = priv->parity;     /* 0=none, 1=odd, 2=even */
  config.bits       = priv->bits;       /* Number of bits (5-9) */
  config.stopbits2  = priv->stopbits2;  /* true: Configure with 2 stop bits instead of 1 */

  ret = imxrt_lpuart_configure(priv->uartbase, &config);

  priv->ie = imxrt_serialin(priv, IMXRT_LPUART_CTRL_OFFSET) | LPUART_ALL_INTS;
  return ret;

#else
  priv->ie = imxrt_serialin(priv, IMXRT_LPUART_CTRL_OFFSET) | LPUART_ALL_INTS;
  return OK;
#endif
}

/****************************************************************************
 * Name: imxrt_shutdown
 *
 * Description:
 *   Disable the UART.  This method is called when the serial
 *   port is closed
 *
 ****************************************************************************/

static void imxrt_shutdown(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;

  /* Disable the UART */

  imxrt_serialout(priv, IMXRT_LPUART_GLOBAL_OFFSET, LPUART_GLOBAL_RST);
}

/****************************************************************************
 * Name: imxrt_attach
 *
 * Description:
 *   Configure the UART to operation in interrupt driven mode.  This method is
 *   called when the serial port is opened.  Normally, this is just after the
 *   the setup() method is called, however, the serial console may operate in
 *   a non-interrupt driven mode during the boot phase.
 *
 *   RX and TX interrupts are not enabled when by the attach method (unless the
 *   hardware supports multiple levels of interrupt enabling).  The RX and TX
 *   interrupts are not enabled until the txint() and rxint() methods are called.
 *
 ****************************************************************************/

static int imxrt_attach(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;
  int ret;

  /* Attach and enable the IRQ */

  ret = irq_attach(priv->irq, imxrt_interrupt, dev);
  if (ret == OK)
    {
      /* Enable the interrupt (RX and TX interrupts are still disabled
       * in the UART
       */

      up_enable_irq(priv->irq);
    }

  return ret;
}

/****************************************************************************
 * Name: imxrt_detach
 *
 * Description:
 *   Detach UART interrupts.  This method is called when the serial port is
 *   closed normally just before the shutdown method is called.  The exception is
 *   the serial console which is never shutdown.
 *
 ****************************************************************************/

static void imxrt_detach(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;

  up_disable_irq(priv->irq);
  irq_detach(priv->irq);
}

/****************************************************************************
 * Name: imxrt_interrupt (and front-ends)
 *
 * Description:
 *   This is the common UART interrupt handler.  It should cal
 *   uart_transmitchars or uart_receivechar to perform the appropriate data
 *   transfers.
 *
 ****************************************************************************/

static int imxrt_interrupt(int irq, void *context, FAR void *arg)
{
  struct uart_dev_s *dev = (struct uart_dev_s *)arg;
  struct imxrt_uart_s *priv;
  uint32_t usr;
  int passes = 0;

  DEBUGASSERT(dev != NULL && dev->priv != NULL);
  priv = (struct imxrt_uart_s *)dev->priv;

  /* Loop until there are no characters to be transferred or,
   * until we have been looping for a long time.
   */

  for (; ; )
    {
      /* Get the current UART status and check for loop
       * termination conditions
       */

      usr = imxrt_serialin(priv, IMXRT_LPUART_STAT_OFFSET);
      usr &= (LPUART_STAT_RDRF | LPUART_STAT_TC);

      if (usr == 0 || passes > 256)
        {
          return OK;
        }

      /* Handle incoming, receive bytes */

      if (usr & LPUART_STAT_RDRF)
        {
          uart_recvchars(dev);
        }

      /* Handle outgoing, transmit bytes */

      if (usr & LPUART_STAT_TC)
        {
          uart_xmitchars(dev);
        }

      /* Keep track of how many times we do this in case there
       * is some hardware failure condition.
       */

      passes++;
    }
}

/****************************************************************************
 * Name: imxrt_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method
 *
 ****************************************************************************/

static int imxrt_ioctl(struct file *filep, int cmd, unsigned long arg)
{
#ifdef CONFIG_SERIAL_TIOCSERGSTRUCT
  struct inode *inode = filep->f_inode;
  struct uart_dev_s *dev   = inode->i_private;
#endif
  int ret   = OK;

  switch (cmd)
    {
#ifdef CONFIG_SERIAL_TIOCSERGSTRUCT
    case TIOCSERGSTRUCT:
      {
         struct imxrt_uart_s *user = (struct imxrt_uart_s *)arg;
         if (!user)
           {
             ret = -EINVAL;
           }
         else
           {
             memcpy(user, dev, sizeof(struct imxrt_uart_s));
           }
       }
       break;
#endif

    case TIOCSBRK:  /* BSD compatibility: Turn break on, unconditionally */
    case TIOCCBRK:  /* BSD compatibility: Turn break off, unconditionally */
    default:
      ret = -ENOTTY;
      break;
    }

  return ret;
}

/****************************************************************************
 * Name: imxrt_receive
 *
 * Description:
 *   Called (usually) from the interrupt level to receive one
 *   character from the UART.  Error bits associated with the
 *   receipt are provided in the return 'status'.
 *
 ****************************************************************************/

static int imxrt_receive(struct uart_dev_s *dev, uint32_t *status)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;
  uint32_t rxd;

  rxd    = imxrt_serialin(priv, IMXRT_LPUART_DATA_OFFSET);
  *status = rxd >> LPUART_DATA_STATUS_SHIFT;
  return (rxd & LPUART_DATA_MASK) >> LPUART_DATA_SHIFT;
}

/****************************************************************************
 * Name: imxrt_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ****************************************************************************/

static void imxrt_rxint(struct uart_dev_s *dev, bool enable)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;
  uint32_t regval = 0;

  /* Enable interrupts for data available at Rx FIFO */

  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      priv->ie |= LPUART_CTRL_RIE;
#endif
    }
  else
    {
      priv->ie &= ~LPUART_CTRL_RIE;
    }

  regval = imxrt_serialin(priv, IMXRT_LPUART_CTRL_OFFSET) | priv->ie;
  imxrt_serialout(priv, IMXRT_LPUART_CTRL_OFFSET, regval);
}

/****************************************************************************
 * Name: imxrt_rxavailable
 *
 * Description:
 *   Return true if the receive fifo is not empty
 *
 ****************************************************************************/

static bool imxrt_rxavailable(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;

  /* Return true is data is ready in the Rx FIFO */

  return ((imxrt_serialin(priv, IMXRT_LPUART_STAT_OFFSET) & LPUART_STAT_RDRF) != 0);
}

/****************************************************************************
 * Name: imxrt_send
 *
 * Description:
 *   This method will send one byte on the UART
 *
 ****************************************************************************/

static void imxrt_send(struct uart_dev_s *dev, int ch)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;
  imxrt_serialout(priv, IMXRT_LPUART_DATA_OFFSET, (uint32_t)ch);
}

/****************************************************************************
 * Name: imxrt_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts
 *
 ****************************************************************************/

static void imxrt_txint(struct uart_dev_s *dev, bool enable)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;
  uint32_t regval = 0;

  /* We won't take an interrupt until the FIFO is completely empty (although
   * there may still be a transmission in progress).
   */

  if (enable)
    {
#ifndef CONFIG_SUPPRESS_SERIAL_INTS
      priv->ie |= LPUART_CTRL_TCIE;
#endif
    }
  else
    {
      priv->ie &= ~LPUART_CTRL_TCIE;
    }

  regval = imxrt_serialin(priv, IMXRT_LPUART_CTRL_OFFSET) | priv->ie;
  imxrt_serialout(priv, IMXRT_LPUART_CTRL_OFFSET, regval);
}

/****************************************************************************
 * Name: imxrt_txready
 *
 * Description:
 *   Return true if the transmit is completed
 *
 ****************************************************************************/

static bool imxrt_txready(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;

  return ((imxrt_serialin(priv, IMXRT_LPUART_STAT_OFFSET) & LPUART_STAT_TC) != 0);
}

/****************************************************************************
 * Name: imxrt_txempty
 *
 * Description:
 *   Return true if the transmit reg is empty
 *
 ****************************************************************************/

static bool imxrt_txempty(struct uart_dev_s *dev)
{
  struct imxrt_uart_s *priv = (struct imxrt_uart_s *)dev->priv;

  return ((imxrt_serialin(priv, IMXRT_LPUART_STAT_OFFSET) & LPUART_STAT_TDRE) != 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: imxrt_earlyserialinit
 *
 * Description:
 *   Performs the low level UART initialization early in debug so that the
 *   serial console will be available during bootup.  This must be called
 *   before up_serialinit.
 *
 ****************************************************************************/

void up_earlyserialinit(void)
{
  /* NOTE: This function assumes that low level hardware configuration
   * -- including all clocking and pin configuration -- was performed by the
   * function imxrt_lowsetup() earlier in the boot sequence.
   */

  /* Enable the console UART.  The other UARTs will be initialized if and
   * when they are first opened.
   */

#ifdef CONSOLE_DEV
  CONSOLE_DEV.isconsole = true;
  imxrt_setup(&CONSOLE_DEV);
#endif
}

/****************************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Register serial console and serial ports.  This assumes
 *   that imxrt_earlyserialinit was called previously.
 *
 ****************************************************************************/

void up_serialinit(void)
{
#ifdef CONSOLE_DEV
  (void)uart_register("/dev/console", &CONSOLE_DEV);
#endif

  /* Register all UARTs */

  (void)uart_register("/dev/ttyS0", &TTYS0_DEV);
#ifdef TTYS1_DEV
  (void)uart_register("/dev/ttyS1", &TTYS1_DEV);
#endif
#ifdef TTYS2_DEV
  (void)uart_register("/dev/ttyS2", &TTYS2_DEV);
#endif
#ifdef TTYS3_DEV
  (void)uart_register("/dev/ttyS3", &TTYS3_DEV);
#endif
#ifdef TTYS4_DEV
  (void)uart_register("/dev/ttyS4", &TTYS4_DEV);
#endif
#ifdef TTYS5_DEV
  (void)uart_register("/dev/ttyS5", &TTYS5_DEV);
#endif
#ifdef TTYS6_DEV
  (void)uart_register("/dev/ttyS6", &TTYS6_DEV);
#endif
#ifdef TTYS7_DEV
  (void)uart_register("/dev/ttyS7", &TTYS7_DEV);
#endif
}

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug  writes
 *
 ****************************************************************************/

int up_putc(int ch)
{
#ifdef HAVE_USART_CONSOLE
  struct lpc54_dev_s *priv = (struct lpc54_dev_s *)CONSOLE_DEV.priv;
  uint32_t ie;

  imxrt_disableuartint(priv, &ie);

  /* Check for LF */

  if (ch == '\n')
    {
      /* Add CR */

      up_lowputc('\r');
    }

  up_lowputc(ch);
  imxrt_restoreuartint(priv, intset);
#endif

  return ch;
}

#else /* USE_SERIALDRIVER */

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug writes
 *
 ****************************************************************************/

int up_putc(int ch)
{
#if CONSOLE_LPUART > 0
  /* Check for LF */

  if (ch == '\n')
    {
      /* Add CR */

      up_lowputc('\r');
    }

  up_lowputc(ch);
#endif

  return ch;
}

#endif /* USE_SERIALDRIVER */
