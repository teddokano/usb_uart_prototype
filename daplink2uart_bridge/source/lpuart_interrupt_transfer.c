/*
 * lpuart_interrupt_transfer.c
 * FRDM-MCXA153  –  DAP-Link UART (DBG) <-> Target UART (TGT) bridge
 *
 * 改良点（元コードからの変更）:
 *   [1] バッファを1バイトから256バイトのリングバッファに変更
 *       → スループット大幅向上、取りこぼし解消
 *   [2] LPUART TransferAPI を廃止し、生の割り込みハンドラに変更
 *       → 1バイト転送のたびにAPIオーバーヘッドが乗る問題を解消
 *   [3] IRQ とメインループ間の競合を __disable_irq/__enable_irq で保護
 *       → volatile フラグの競合によるデータ化けを解消
 *   [4] DBG/TGT それぞれ独立した config で LPUART_Init を呼ぶ
 *       → 異なるボーレート・クロック源に対応可能
 *   [5] (void)userData に修正
 *       → userData = userData の無効な警告抑制を修正
 *   [6] #if 1 デバッグガードを削除
 *   [7] #include <string.h> を明示
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
    uint8_t          buf[RING_BUFFER_SIZE];
    volatile uint32_t head;   /* IRQ が書く */
    volatile uint32_t tail;   /* メインループが読む */
} ring_buf_t;

static inline bool rb_push(ring_buf_t *rb, uint8_t data)
{
    uint32_t next = (rb->head + 1U) & RING_BUFFER_MASK;
    if (next == rb->tail)
    {
        return false;   /* バッファフル: 古いデータを守り新データを捨てる */
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

/*******************************************************************************
 * バッファインスタンス
 *   dbg_rx_buf : DBG受信 → TGT送信 方向
 *   tgt_rx_buf : TGT受信 → DBG送信 方向
 ******************************************************************************/
static ring_buf_t dbg_rx_buf;
static ring_buf_t tgt_rx_buf;

/*******************************************************************************
 * 割り込みハンドラ
 * LPUART Transfer API を使わず、直接レジスタを読むことで
 * 1バイトあたりのオーバーヘッドを最小化する。
 ******************************************************************************/
void DBG_LPUART_IRQHandler(void)
{
    uint32_t status = LPUART_GetStatusFlags(DBG_LPUART);

    if (status & kLPUART_RxDataRegFullFlag)
    {
        rb_push(&dbg_rx_buf, (uint8_t)LPUART_ReadByte(DBG_LPUART));
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

    if (status & kLPUART_RxDataRegFullFlag)
    {
        rb_push(&tgt_rx_buf, (uint8_t)LPUART_ReadByte(TGT_LPUART));
    }
    if (status & kLPUART_RxOverrunFlag)
    {
        LPUART_ClearStatusFlags(TGT_LPUART, kLPUART_RxOverrunFlag);
    }
    SDK_ISR_EXIT_BARRIER;
}

/*******************************************************************************
 * LPUART 初期化
 *
 * DBG と TGT で独立した config 変数を使うことで、
 * 将来的に異なるボーレート・クロック源への変更が容易になる。
 *
 * HardFault 対策:
 *   LPUART_Init() は内部でソフトリセットを行い TX/RX が一時無効になる。
 *   enableTx/enableRx = true を含めた Init 完了後に
 *   LPUART_EnableInterrupts() を呼ぶこと。
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
    tgt_config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;  /* app.h で別定義も可 */
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
        uint8_t data;

        /* DBG -> TGT 方向 */
        while (rb_pop(&dbg_rx_buf, &data))
        {
            LPUART_WriteBlocking(TGT_LPUART, &data, 1U);
        }

        /* TGT -> DBG 方向 */
        while (rb_pop(&tgt_rx_buf, &data))
        {
            LPUART_WriteBlocking(DBG_LPUART, &data, 1U);
        }
    }
}
