# usb_uart_prototype

## What is this?

Repository for samples of USB-UART bridge application.  

### What it does

The application that forward UART data between the DAP-Link interface and the Arduino shield connector (D0/D1 pins).  
The USB CDC-ACM ↔ UART conversion is handled by the on-board DAP-Link chip, not by the MCU.  
The MCU's role is to forward data between the DAP-Link UART and a second UART connected to the Arduino shield D0 (RX) / D1 (TX) pins.  

![connectors](https://github.com/teddokano/usb_uart_prototype/blob/main/docs/img/connectors.png)
*Board connectors: MCU-Link USB (J15) and Arduino shield D0/D1 (J1-2, J1-4)*


```
PC
 |
 | USB (CDC-ACM)
 |
DAP-Link chip        ← USB ↔ UART conversion (done by DAP-Link, not MCU)
 |
 | UART (DAP-Link side)
 |
MCU                  ← forwards data between the two UARTs
 |
 | UART (user side)
 |
Arduino D0 / D1 pins ← connect your target device here
```
*Signal flow: USB to Arduino D0/D1 pins*

### How to build
This repository can be imported into MCUXpresso IDE and MCUXpresso for VSC.  
Just import, build and run.  

> Note:
> Except `daplink2uart_bridge_zephyr` project. It is not for MCUXpresso.   
> It needs to be handled in Zephyr environment. 


### Quick check
Short the D0 and D1 pins together to create a loopback path.  
Open a terminal at 115200 baud on the MCU-Link USB port (J15) — typed characters should echo back immediately.

## Projects
 There are some projects implemented in different ways.   
 
### * daplink2uart_bridge
Basic implementation using the MCUXpresso SDK API.  
Bridges MCU-Link USB connector (J15) ↔ Arduino shield D0/D1 (J1-2, J1-4).

**Board:** FRDM-MCXA153  
**Environment:** MCUXpresso IDE or VS Code  

### * daplink2uart_bridge_cpp
Same functionality as `daplink2uart_bridge` rewritten in C++ for better code maintenancability (using MCUXpresso SDK API).  
Bridges MCU-Link USB connector (J15) ↔ Arduino shield D0/D1 (J1-2, J1-4).

**Board:** FRDM-MCXA153  
**Environment:** MCUXpresso IDE or VS Code  


### * daplink2uart_bridge_r01lib
Same functionality as `daplink2uart_bridge` rewritten in C++ using r01lib library API to have good extensibility.  
Bridges MCU-Link USB connector (J15) ↔ Arduino shield D0/D1 (J1-2, J1-4).

**Board:** FRDM-MCXA153  
**Environment:** MCUXpresso IDE or VS Code  


### * daplink2uart_bridge_zephyr
Zephyr RTOS implementation. Written at a high abstraction level using Zephyr's UART API and DeviceTree, making it portable across boards with only overlay changes.  
Bridges MCU-Link USB connector (J15) ↔ Arduino shield D0/D1 (J1-2, J1-4).

For build instructions, see [`README.md`](https://github.com/teddokano/usb_uart_prototype/blob/main/daplink2uart_bridge_zephyr/README.md) in that project folder.  

**Boards:** FRDM-MCXA153, FRDM-MCXN947  
**Environment:** Zephyr development environment (`west`)  

### * __test_too__hello_world_generator
Data TX app to test brige RX capability.  
This app transmit test data from D1 pin continuously.  

**Board:** FRDM-MCXA153  
**Environment:** MCUXpresso IDE or VS Code  

### * __prototyping__dual_uart
Experimental project written at an early stage to verify two independent UART ports.  
Does **NOT** bridge data — each UART simply loops back independently.  
Not intended for real use.

### * _r01lib_frdm_mcxa153
Library project: r01lib. This project cannot run by itself. This project is linked from `daplink2uart_bridge_r01lib` project.  
