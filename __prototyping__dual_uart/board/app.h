/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
#if 1
#define DBG_LPUART            LPUART0
#define DBG_LPUART_CLK_FREQ   (BOARD_DEBUG_UART_CLK_FREQ)
#define DBG_LPUART_IRQn       LPUART0_IRQn
#define DBG_LPUART_IRQHandler LPUART0_IRQHandler

#define TGT_LPUART            LPUART2
#define TGT_LPUART_CLK_FREQ   (BOARD_DEBUG_UART_CLK_FREQ)
#define TGT_LPUART_IRQn       LPUART2_IRQn
#define TGT_LPUART_IRQHandler LPUART2_IRQHandler
#else
#define TGT_LPUART            LPUART0
#define TGT_LPUART_CLK_FREQ   (BOARD_DEBUG_UART_CLK_FREQ)
#define TGT_LPUART_IRQn       LPUART0_IRQn
#define TGT_LPUART_IRQHandler LPUART0_IRQHandler

#define DBG_LPUART            LPUART2
#define DBG_LPUART_CLK_FREQ   (BOARD_DEBUG_UART_CLK_FREQ)
#define DBG_LPUART_IRQn       LPUART2_IRQn
#define DBG_LPUART_IRQHandler LPUART2_IRQHandler
#endif
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
