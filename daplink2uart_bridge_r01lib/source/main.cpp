/*
 *  main_serial_bridge.cpp
 *
 *  UART bridge sample for FRDM-MCXA153 using r01lib Serial class
 *
 *  Forwards data bidirectionally between two UART channels:
 *
 *    PC side  : LPUART0  USBTX(P0_3) / USBRX(P0_2)   115200 baud
 *    Target   : LPUART2  MB_TX(P3_15) / MB_RX(P3_14)  115200 baud
 *
 *  Data flow:
 *    PC    --> FRDM (LPUART0 RX) --> forwarded to LPUART2 TX --> Target
 *    Target --> FRDM (LPUART2 RX) --> forwarded to LPUART0 TX --> PC
 *
 *  All forwarding is done inside RX interrupt callbacks.
 *  The TX ring buffer in Serial ensures putc() is non-blocking,
 *  so calling it from an ISR is safe.
 *
 *  Wiring:
 *    FRDM P0_3 (USBTX) --> USB-Serial adapter RX  (PC side)
 *    FRDM P0_2 (USBRX) --> USB-Serial adapter TX  (PC side)
 *    FRDM P3_15 (MB_TX) --> Target device RX
 *    FRDM P3_14 (MB_RX) --> Target device TX
 *    GND                --> common GND
 *
 *  Prerequisites:
 *    - r01lib project with semihosting (SEMIHOST_OPERATION defined)
 *    - Serial.h / Serial.cpp in source/r01lib/
 *    - "#include "Serial.h"" added to r01lib.h
 */

#include "r01lib.h"

volatile int	count	= 0;

// ---- 2 UART channels -------------------------------------------------------

Serial  pc(   USBTX, USBRX, 115200 );   // PC side
Serial  uart( D1,    D0,    115200 );   // Target
//Serial  uart( MB_TX, MB_RX, 115200 );   // Target

BusOut rgb( RED, GREEN, BLUE );

// ---- RX callbacks ----------------------------------------------------------


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvolatile"

// PC → Target: byte received from PC, forward to uart
void on_pc_rx( void )
{
	int c;

	if ( -1 != ( c = pc.getc() ) )
	{
		uart.putc( c );
		count++;
	}
}

// Target → PC: byte received from Target, forward to pc
void on_uart_rx( void )
{
	int c;

	if ( -1 != ( c = uart.getc() ) )
	{
		pc.putc( c );
		count++;
	}
}

#pragma GCC diagnostic pop

// ---- main ------------------------------------------------------------------

int main( void )
{
	int	prev_count	= -1;

	pc.attach  ( on_pc_rx,   Serial::RxIrq );
	uart.attach( on_uart_rx, Serial::RxIrq );

	// Heartbeat: blink LED to show the bridge is alive
	while ( true )
	{
		if ( prev_count != count )
		{
			prev_count	= count;
			rgb			= count & 0x7;
		}
	}
}
