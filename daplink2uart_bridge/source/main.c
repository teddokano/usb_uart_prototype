/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 *
 * FRDM-MCXA153  –  DAP-Link UART (DBG) <-> Target UART (TGT) bridge
 * C version
 */

#include <string.h>
#include "board.h"
#include "app.h"
#include "fsl_lpuart.h"

/*******************************************************************************
 * 設定
 ******************************************************************************/

/** リングバッファサイズ（2の累乗であること） */
#define RING_BUFFER_SIZE    256U
#define RING_BUFFER_MASK    (RING_BUFFER_SIZE - 1U)

/*******************************************************************************
 * リングバッファ型
 ******************************************************************************/
typedef struct {
    uint8_t           buf[RING_BUFFER_SIZE];
    volatile uint32_t head;   /* 書き込み側が更新 */
    volatile uint32_t tail;   /* 読み出し側が更新 */
} ring_buf_t;

static inline bool rb_push(ring_buf_t *rb, uint8_t data)
{
    uint32_t next = (rb->head + 1U) & RING_BUFFER_MASK;
    if (next == rb->tail)
    {
        return false;   /* バッファフル: 新データを捨てる */
    }
    rb->buf[rb->head] = data;
    rb->head = next;
    return true;
}

static inline bool rb_pop(ring_buf_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail)
    {
        return false;   /* 空 */
    }
    *data = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1U) & RING_BUFFER_MASK;
    return true;
}

static inline bool rb_empty(const ring_buf_t *rb)
{
    return rb->head == rb->tail;
}

/*******************************************************************************
 * バッファインスタンス
 *   dbg_rx_buf : DBG受信バッファ（DBG RX割り込み → TGT TX割り込みで送信）
 *   tgt_rx_buf : TGT受信バッファ（TGT RX割り込み → DBG TX割り込みで送信）
 ******************************************************************************/
static ring_buf_t dbg_rx_buf;  /* DBGから受信したデータ → TGTへ送信 */
static ring_buf_t tgt_rx_buf;  /* TGTから受信したデータ → DBGへ送信 */

/*******************************************************************************
 * 割り込みハンドラ
 *
 * 設計方針:
 *   RX割り込み: 受信バイトを相手側のTXバッファに積み、相手側TX割り込みを有効化
 *   TX割り込み: バッファからバイトを取り出して送信。
 *               バッファが空になったらTX割り込みを無効化（CPUを無駄に起こさない）
 *
 * これにより LPUART_WriteBlocking() のような送信待ちポーリングが不要になり、
 * 受信取りこぼしを防止できる。
 ******************************************************************************/
void DBG_LPUART_IRQHandler(void)
{
    uint32_t status = LPUART_GetStatusFlags(DBG_LPUART);

    /* RX: 受信データを tgt_rx_buf に積み、TGT TX割り込みを起動 */
    if (status & kLPUART_RxDataRegFullFlag)
    {
        rb_push(&tgt_rx_buf, (uint8_t)LPUART_ReadByte(DBG_LPUART));
        LPUART_EnableInterrupts(TGT_LPUART, kLPUART_TxDataRegEmptyInterruptEnable);
    }

    /* TX: dbg_rx_buf から送信。空になったらTX割り込みを無効化 */
    if (status & kLPUART_TxDataRegEmptyFlag)
    {
        uint8_t data;
        if (rb_pop(&dbg_rx_buf, &data))
        {
            LPUART_WriteByte(DBG_LPUART, data);
        }
        else
        {
            LPUART_DisableInterrupts(DBG_LPUART, kLPUART_TxDataRegEmptyInterruptEnable);
        }
    }

    if (status & kLPUART_RxOverrunFlag)
    {
        LPUART_ClearStatusFlags(DBG_LPUART, kLPUART_RxOverrunFlag);
    }
    SDK_ISR_EXIT_BARRIER;
}

void TGT_LPUART_IRQHandler(void)
{
    uint32_t status = LPUART_GetStatusFlags(TGT_LPUART);

    /* RX: 受信データを dbg_rx_buf に積み、DBG TX割り込みを起動 */
    if (status & kLPUART_RxDataRegFullFlag)
    {
        rb_push(&dbg_rx_buf, (uint8_t)LPUART_ReadByte(TGT_LPUART));
        LPUART_EnableInterrupts(DBG_LPUART, kLPUART_TxDataRegEmptyInterruptEnable);
    }

    /* TX: tgt_rx_buf から送信。空になったらTX割り込みを無効化 */
    if (status & kLPUART_TxDataRegEmptyFlag)
    {
        uint8_t data;
        if (rb_pop(&tgt_rx_buf, &data))
        {
            LPUART_WriteByte(TGT_LPUART, data);
        }
        else
        {
            LPUART_DisableInterrupts(TGT_LPUART, kLPUART_TxDataRegEmptyInterruptEnable);
        }
    }

    if (status & kLPUART_RxOverrunFlag)
    {
        LPUART_ClearStatusFlags(TGT_LPUART, kLPUART_RxOverrunFlag);
    }
    SDK_ISR_EXIT_BARRIER;
}

/*******************************************************************************
 * LPUART 初期化
 ******************************************************************************/
static void uart_init(void)
{
    lpuart_config_t dbg_config;
    lpuart_config_t tgt_config;

    /* --- DBG UART --- */
    LPUART_GetDefaultConfig(&dbg_config);
    dbg_config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    dbg_config.enableTx     = true;
    dbg_config.enableRx     = true;
    LPUART_Init(DBG_LPUART, &dbg_config, DBG_LPUART_CLK_FREQ);
    LPUART_EnableInterrupts(DBG_LPUART, kLPUART_RxDataRegFullInterruptEnable);

	EnableIRQ(DBG_LPUART_IRQn);

    /* --- TGT UART --- */
    LPUART_GetDefaultConfig(&tgt_config);

	tgt_config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
	tgt_config.enableTx     = true;
    tgt_config.enableRx     = true;
    LPUART_Init(TGT_LPUART, &tgt_config, TGT_LPUART_CLK_FREQ);
    LPUART_EnableInterrupts(TGT_LPUART, kLPUART_RxDataRegFullInterruptEnable);

	EnableIRQ(TGT_LPUART_IRQn);
}

/*******************************************************************************
 * main
 ******************************************************************************/

int main(void)
{
    BOARD_InitHardware();
    uart_init();

    while (1)
    {
        __WFI();    /* 割り込み待ち（低消費電力） */
    }
}
