/*
 * main.c  –  UART Bridge for FRDM-MCXA153 (Zephyr版)
 *
 * DAP-Link UART (lpuart0) <-> User UART (lpuart2) を双方向ブリッジします。
 *
 *   uart0 (lpuart0): P0_2=RX / P0_3=TX  ← DAP-Link VCOM (デフォルト設定済)
 *   uart2 (lpuart2): P1_4=RX / P1_5=TX  ← User UART (app.overlay で有効化)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

/*******************************************************************************
 * デバイス取得 (DeviceTree ノードラベルから)
 ******************************************************************************/
#define DBG_UART DEVICE_DT_GET(DT_CHOSEN(uart_bridge_dbg))
#define TGT_UART DEVICE_DT_GET(DT_CHOSEN(uart_bridge_tgt))

static const struct device *dbg_uart;
static const struct device *tgt_uart;

/*******************************************************************************
 * リングバッファ (Zephyr 組み込みマクロ)
 ******************************************************************************/
#define RING_BUF_SIZE   256

RING_BUF_DECLARE(dbg_rx_buf, RING_BUF_SIZE);   /* DBG受信 -> TGT送信 */
RING_BUF_DECLARE(tgt_rx_buf, RING_BUF_SIZE);   /* TGT受信 -> DBG送信 */

/*******************************************************************************
 * UART IRQ コールバック
 *
 * DBG / TGT で共通のコールバック関数を使う。
 * user_data に転送先リングバッファのポインタを渡すことで
 * 1つの関数で両チャンネルをカバーする。
 *
 * Zephyr UART割り込みコールバックの必須手順:
 *   1. uart_irq_update()    – 割り込みステータスをキャッシュ更新（必須）
 *   2. uart_irq_rx_ready()  – RX データが届いているか確認
 *   3. uart_fifo_read()     – FIFO が空になるまでループで読み出す
 ******************************************************************************/
static void uart_cb(const struct device *dev, void *user_data)
{
    struct ring_buf *rb = (struct ring_buf *)user_data;
    uint8_t byte;

    if (!uart_irq_update(dev)) {
        return;
    }

    while (uart_irq_rx_ready(dev)) {
        while (uart_fifo_read(dev, &byte, 1) == 1) {
            /* フル時はこのバイトを捨てる（古いデータを保持） */
            ring_buf_put(rb, &byte, 1);
        }
    }
}

/*******************************************************************************
 * 初期化
 ******************************************************************************/
static int uart_bridge_init(void)
{
    if (!device_is_ready(dbg_uart)) {
        printk("DBG UART (lpuart0) not ready\n");
        return -ENODEV;
    }
    if (!device_is_ready(tgt_uart)) {
        printk("TGT UART (lpuart2) not ready\n");
        return -ENODEV;
    }

    /*
     * コールバック登録:
     *   dbg_uart の受信データは dbg_rx_buf へ、
     *   tgt_uart の受信データは tgt_rx_buf へ蓄積する。
     *   メインループで各バッファを反対側の UART に送出する。
     */
    uart_irq_callback_user_data_set(dbg_uart, uart_cb, &dbg_rx_buf);
    uart_irq_callback_user_data_set(tgt_uart, uart_cb, &tgt_rx_buf);

    uart_irq_rx_enable(dbg_uart);
    uart_irq_rx_enable(tgt_uart);

    return 0;
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
	dbg_uart = DEVICE_DT_GET(DT_CHOSEN(uart_bridge_dbg));
	tgt_uart = DEVICE_DT_GET(DT_CHOSEN(uart_bridge_tgt));

	if (!device_is_ready(dbg_uart) || !device_is_ready(tgt_uart)) {
		return -1;
	}

    if (uart_bridge_init() != 0) {
        return -1;
    }

    uint8_t byte;

    while (1) {
        /* DBG -> TGT */
        while (ring_buf_get(&dbg_rx_buf, &byte, 1) == 1) {
            uart_poll_out(tgt_uart, byte);
        }

        /* TGT -> DBG */
        while (ring_buf_get(&tgt_rx_buf, &byte, 1) == 1) {
            uart_poll_out(dbg_uart, byte);
        }
    }
}
