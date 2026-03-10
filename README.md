# usb_uart_prototype

## What is this?

Repository for samples of USB-UART bridge application.  
The sample code itselves are not bridging data between USB and UART.  
The USB communication device class (CDC) to UART conversion is done by on-board debug interface chip (DAP-LINK chip).  
The UART converted data is forward to Arduino shield socket D0(RX) and D1(TX) pins on the board.  

Target: FRDM-MCXA153   
Environment: MCUXpresso IDE or VSC   

## Projects
 There are some projects implemented in different ways.   
 
 ### dual_uart
This sample does nothing about bridging but just looping back debug chip side UART and D0/D1 pins independently

### daplink2uart_bridge
Basic version using MCUXpresso SDK API  

### daplink2uart_bridge_cpp
Same function as `daplink2uart_bridge` but written in C++ for better code maintenancability (using MCUXpresso SDK API)

### daplink2uart_bridge_zephyr
Using Zephyr OS. The code itself is written in highly abstracted level and runs on FRDM-MCXA153 and FRDM-MCXN947.  
Refer to `README.md` in that folder.  

