/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 *
 * FRDM-MCXA153  –  DAP-Link UART (DBG) <-> Target UART (TGT) bridge
 * C++ version
 */

#include "board.h"
#include "fsl_lpuart.h"
extern "C" {
#include "app.h"
}

/*******************************************************************************
 * RingBuffer<N> : テンプレートで任意サイズのリングバッファ
 * 単一プロデューサ・単一コンシューマなら排他制御不要。
 ******************************************************************************/
template<uint32_t N>
class RingBuffer
{
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    uint8_t           buf_[N];
    volatile uint32_t head_ = 0;
    volatile uint32_t tail_ = 0;

public:
    void push(uint8_t data)
    {
        uint32_t next = (head_ + 1) & (N - 1);
        if (next != tail_)          /* フル時は新データを捨てる */
        {
            buf_[head_] = data;
            head_ = next;
        }
    }

    bool pop(uint8_t &data)
    {
        if (head_ == tail_) return false;
        data  = buf_[tail_];
        tail_ = (tail_ + 1) & (N - 1);
        return true;
    }
};

/*******************************************************************************
 * UartChannel : 1つの LPUART インスタンスをラップするクラス
 *
 * 設計方針:
 *   handleIrq(peer) を呼ぶ際に転送先チャンネルへの参照を渡す。
 *   RX割り込み: 受信バイトを peer.enqueue() でpeerのTXバッファに積む。
 *   TX割り込み: TXバッファからバイトを取り出して送信。
 *               バッファが空になったらTX割り込みを無効化。
 *   メインループ: WFI のみ（何もしない）。
 ******************************************************************************/
class UartChannel
{
    LPUART_Type *const uart_;
    IRQn_Type          irq_;
    RingBuffer<256>    tx_;     /* TX送信バッファ */

public:
    constexpr UartChannel(LPUART_Type *uart) : uart_(uart), irq_() {}

    void init(uint32_t baudrate, uint32_t clkFreq, IRQn_Type irq)
    {
        irq_ = irq;
        lpuart_config_t cfg;
        LPUART_GetDefaultConfig(&cfg);
        cfg.baudRate_Bps = baudrate;
        cfg.enableTx     = true;
        cfg.enableRx     = true;
        LPUART_Init(uart_, &cfg, clkFreq);

		/* RX割り込みのみ有効化。TX割り込みはデータが来たとき有効化。 */
        LPUART_EnableInterrupts(uart_, kLPUART_RxDataRegFullInterruptEnable);
        EnableIRQ(irq_);
    }

    /* 相手チャンネルのRX割り込みから呼ばれる。
     * TXバッファに積んでTX割り込みを有効化するだけ。ブロックしない。 */
    void enqueue(uint8_t data)
    {
        tx_.push(data);
        LPUART_EnableInterrupts(uart_, kLPUART_TxDataRegEmptyInterruptEnable);
    }

    /* peerはRXデータの転送先チャンネル */
    void handleIrq(UartChannel &peer)
    {
        uint32_t status = LPUART_GetStatusFlags(uart_);

        /* RX: 受信バイトをpeerのTXバッファに積む */
        if (status & kLPUART_RxDataRegFullFlag)
        {
            peer.enqueue(LPUART_ReadByte(uart_));
        }

        /* TX: TXバッファからバイトを送信。空になったらTX割り込みを無効化 */
        if (status & kLPUART_TxDataRegEmptyFlag)
        {
            uint8_t data;
            if (tx_.pop(data))
            {
                LPUART_WriteByte(uart_, data);
            }
            else
            {
                LPUART_DisableInterrupts(uart_, kLPUART_TxDataRegEmptyInterruptEnable);
            }
        }

        if (status & kLPUART_RxOverrunFlag)
        {
            LPUART_ClearStatusFlags(uart_, kLPUART_RxOverrunFlag);
        }
        SDK_ISR_EXIT_BARRIER;
    }
};

/*******************************************************************************
 * チャンネルインスタンス
 ******************************************************************************/
static UartChannel dbg(DBG_LPUART);
static UartChannel tgt(TGT_LPUART);

/*******************************************************************************
 * 割り込みハンドラ
 ******************************************************************************/
extern "C" void DBG_LPUART_IRQHandler(void) { dbg.handleIrq(tgt); }
extern "C" void TGT_LPUART_IRQHandler(void) { tgt.handleIrq(dbg); }

/*******************************************************************************
 * main
 *
 * 全処理を割り込みで完結させるため、メインループは WFI のみ。
 ******************************************************************************/
int main(void)
{
    BOARD_InitHardware();

    dbg.init(BOARD_DEBUG_UART_BAUDRATE, DBG_LPUART_CLK_FREQ, DBG_LPUART_IRQn);
    tgt.init(BOARD_DEBUG_UART_BAUDRATE, TGT_LPUART_CLK_FREQ, TGT_LPUART_IRQn);

    while (true)
    {
        __WFI();    /* 割り込み待ち（低消費電力） */
    }
}
