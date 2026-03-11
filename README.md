# usb_uart_prototype

## What is this?

Repository for samples of USB-UART bridge application.  
The sample code itselves are not bridging data between USB and UART.  
The USB communication device class (CDC) to UART conversion is done by on-board debug interface chip (DAP-LINK chip).  
The UART converted data is forward to Arduino shield socket D0(RX) and D1(TX) pins on the board.  

![connectors](https://github.com/teddokano/usb_uart_prototype/blob/main/docs/img/connectors.png)

Target: FRDM-MCXA153   
Environment: MCUXpresso IDE or VSC   

## Projects
 There are some projects implemented in different ways.   
 
### daplink2uart_bridge
Basic version using MCUXpresso SDK API.  
Performs USB CDC-ACM adapter on MCU-Link USB connector (J15) and D0/D1 pins (J1-2, J1-4) on Arduino shield socket.    

### daplink2uart_bridge_cpp
Same function as `daplink2uart_bridge` but written in C++ for better code maintenancability (using MCUXpresso SDK API).  
Performs USB CDC-ACM adapter on MCU-Link USB connector (J15) and D0/D1 pins (J1-2, J1-4) on Arduino shield socket.    


### daplink2uart_bridge_zephyr
Using Zephyr OS. The code itself is written in highly abstracted level and runs on FRDM-MCXA153 and FRDM-MCXN947.  
Performs USB CDC-ACM adapter on MCU-Link USB connector (J15) and D0/D1 pins (J1-2, J1-4) on Arduino shield socket.    

This project is intended to be built by Zephyr development environment. 
Refer to [`README.md`](https://github.com/teddokano/usb_uart_prototype/blob/main/daplink2uart_bridge_zephyr/README.md) in that project folder.  

### dual_uart
Just an experimental project to check functionality of two UART ports.  
This sample does nothing about bridging but just looping back debug chip side UART and D0/D1 pins independently
