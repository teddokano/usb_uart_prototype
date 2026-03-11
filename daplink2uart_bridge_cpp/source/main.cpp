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
 * IRQ(push) とメインループ(pop) で head/tail を分離しているため
 * 単一プロデューサ・単一コンシューマなら排他制御不要。
 ******************************************************************************/
template<uint32_t N>
class RingBuffer
{
    static_assert((N & (N - 1)) == 0, "N must be a power of 2");

    uint8_t          buf_[N];
    volatile uint32_t head_ = 0;   /* IRQ が書く */
    volatile uint32_t tail_ = 0;   /* メインループが読む */

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
 ******************************************************************************/
class UartChannel
{
    LPUART_Type *const uart_;

public:
    RingBuffer<256> rx;     /* IRQHandler から直接アクセスさせるため public */

    constexpr UartChannel(LPUART_Type *uart) : uart_(uart) {}

    void init(uint32_t baudrate, uint32_t clkFreq, IRQn_Type irq)
    {
        lpuart_config_t cfg;
        LPUART_GetDefaultConfig(&cfg);
        cfg.baudRate_Bps = baudrate;
        cfg.enableTx     = true;
        cfg.enableRx     = true;
        LPUART_Init(uart_, &cfg, clkFreq);
        LPUART_EnableInterrupts(uart_, kLPUART_RxDataRegFullInterruptEnable);
        EnableIRQ(irq);
    }

    void handleIrq()
    {
        uint32_t status = LPUART_GetStatusFlags(uart_);
        if (status & kLPUART_RxDataRegFullFlag)
            rx.push(LPUART_ReadByte(uart_));
        if (status & kLPUART_RxOverrunFlag)
            LPUART_ClearStatusFlags(uart_, kLPUART_RxOverrunFlag);
        SDK_ISR_EXIT_BARRIER;
    }

    void sendByte(uint8_t data)
    {
        LPUART_WriteBlocking(uart_, &data, 1U);
    }
};

/*******************************************************************************
 * チャンネルインスタンス（IRQHandler からアクセスするため外部リンケージ）
 ******************************************************************************/
static UartChannel dbg(DBG_LPUART);
static UartChannel tgt(TGT_LPUART);

/*******************************************************************************
 * 割り込みハンドラ
 ******************************************************************************/
extern "C" void DBG_LPUART_IRQHandler(void) { dbg.handleIrq(); }
extern "C" void TGT_LPUART_IRQHandler(void) { tgt.handleIrq(); }

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    BOARD_InitHardware();

    dbg.init(BOARD_DEBUG_UART_BAUDRATE, DBG_LPUART_CLK_FREQ, DBG_LPUART_IRQn);
    tgt.init(BOARD_DEBUG_UART_BAUDRATE, TGT_LPUART_CLK_FREQ, TGT_LPUART_IRQn);

    while (true)
    {
        uint8_t data;

        /* DBG -> TGT */
        while (dbg.rx.pop(data)) tgt.sendByte(data);

        /* TGT -> DBG */
        while (tgt.rx.pop(data)) dbg.sendByte(data);
    }
}
