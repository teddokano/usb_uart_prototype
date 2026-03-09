/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "app.h"
#include "fsl_lpuart.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ECHO_BUFFER_LENGTH 1

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* LPUART user callback */
void dbg_LPUART_UserCallback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData);
void tgt_LPUART_UserCallback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData);

/*******************************************************************************
 * Variables
 ******************************************************************************/
lpuart_handle_t dbg_lpuartHandle;

uint8_t dbg_txBuffer[ECHO_BUFFER_LENGTH] = {0};
uint8_t dbg_rxBuffer[ECHO_BUFFER_LENGTH] = {0};
volatile bool dbg_rxBufferEmpty            = true;
volatile bool dbg_txBufferFull             = false;
volatile bool dbg_txOnGoing                = false;
volatile bool dbg_rxOnGoing                = false;

lpuart_handle_t tgt_lpuartHandle;

uint8_t tgt_txBuffer[ECHO_BUFFER_LENGTH] = {0};
uint8_t tgt_rxBuffer[ECHO_BUFFER_LENGTH] = {0};
volatile bool tgt_rxBufferEmpty            = true;
volatile bool tgt_txBufferFull             = false;
volatile bool tgt_txOnGoing                = false;
volatile bool tgt_rxOnGoing                = false;

/*******************************************************************************
 * Code
 ******************************************************************************/
/* LPUART user callback */
void dbg_LPUART_UserCallback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData)
{
	userData = userData;

	if (kStatus_LPUART_TxIdle == status)
	{
		dbg_txBufferFull = false;
		dbg_txOnGoing    = false;
	}

	if (kStatus_LPUART_RxIdle == status)
	{
		dbg_rxBufferEmpty = false;
		dbg_rxOnGoing     = false;
	}
}

/* LPUART user callback */
void tgt_LPUART_UserCallback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData)
{
	userData = userData;

	if (kStatus_LPUART_TxIdle == status)
	{
		tgt_txBufferFull = false;
		tgt_txOnGoing    = false;
	}

	if (kStatus_LPUART_RxIdle == status)
	{
		tgt_rxBufferEmpty = false;
		tgt_rxOnGoing     = false;
	}
}

int main(void)
{
    lpuart_config_t config;
    lpuart_transfer_t xfer;
	lpuart_transfer_t dbg_sendXfer;
	lpuart_transfer_t dbg_receiveXfer;
	lpuart_transfer_t tgt_sendXfer;
	lpuart_transfer_t tgt_receiveXfer;

    BOARD_InitHardware();

    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

	LPUART_Init(DBG_LPUART, &config, DBG_LPUART_CLK_FREQ);
	LPUART_TransferCreateHandle(DBG_LPUART, &dbg_lpuartHandle, dbg_LPUART_UserCallback, NULL);

	LPUART_Init(TGT_LPUART, &config, TGT_LPUART_CLK_FREQ);
	LPUART_TransferCreateHandle(TGT_LPUART, &tgt_lpuartHandle, tgt_LPUART_UserCallback, NULL);

    /* Start to echo. */
    dbg_sendXfer.data        = dbg_txBuffer;
    dbg_sendXfer.dataSize    = ECHO_BUFFER_LENGTH;
    dbg_receiveXfer.data     = dbg_rxBuffer;
    dbg_receiveXfer.dataSize = ECHO_BUFFER_LENGTH;

	tgt_sendXfer.data        = tgt_txBuffer;
	tgt_sendXfer.dataSize    = ECHO_BUFFER_LENGTH;
	tgt_receiveXfer.data     = tgt_rxBuffer;
	tgt_receiveXfer.dataSize = ECHO_BUFFER_LENGTH;

    while (1)
    {
		//	DBG
        if ((!dbg_rxOnGoing) && dbg_rxBufferEmpty)
        {
            dbg_rxOnGoing = true;
            LPUART_TransferReceiveNonBlocking(DBG_LPUART, &dbg_lpuartHandle, &dbg_receiveXfer, NULL);
        }

        if ((!dbg_txOnGoing) && dbg_txBufferFull)
        {
            dbg_txOnGoing = true;
            LPUART_TransferSendNonBlocking(DBG_LPUART, &dbg_lpuartHandle, &dbg_sendXfer);
        }

        if ((!dbg_rxBufferEmpty) && (!tgt_txBufferFull))
        {
//      	memcpy(dbg_txBuffer, dbg_rxBuffer, ECHO_BUFFER_LENGTH);
            memcpy(tgt_txBuffer, dbg_rxBuffer, ECHO_BUFFER_LENGTH);
            dbg_rxBufferEmpty = true;
            tgt_txBufferFull  = true;
        }
#if 1
		//	TGT
		if ((!tgt_rxOnGoing) && tgt_rxBufferEmpty)
		{
			tgt_rxOnGoing = true;
			LPUART_TransferReceiveNonBlocking(TGT_LPUART, &tgt_lpuartHandle, &tgt_receiveXfer, NULL);
		}

		if ((!tgt_txOnGoing) && tgt_txBufferFull)
		{
			tgt_txOnGoing = true;
			LPUART_TransferSendNonBlocking(TGT_LPUART, &tgt_lpuartHandle, &tgt_sendXfer);
		}

		if ((!tgt_rxBufferEmpty) && (!dbg_txBufferFull))
		{
//			memcpy(tgt_txBuffer, tgt_rxBuffer, ECHO_BUFFER_LENGTH);
			memcpy(dbg_txBuffer, tgt_rxBuffer, ECHO_BUFFER_LENGTH);
			tgt_rxBufferEmpty = true;
			dbg_txBufferFull  = true;
		}
#endif
	
	}
}
