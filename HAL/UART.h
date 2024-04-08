/*
 * UART.h
 *
 *  Created on: Dec 31, 2019
 *      Author: Matthew Zhong
 *  Supervisor: Leyla Nazhand-Ali
 */

#ifndef HAL_UART_H_
#define HAL_UART_H_

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// Ever since the new API update in mid 2019, the UART configuration struct's
// name changed from [eUSCI_UART_Config] to [eUSCI_UART_ConfigV1]. If your code
// does not compile, uncomment the alternate #define and comment the original
// one.
// -----------------------------------------------------------------------------
// #define UART_Config eUSCI_UART_Config
#define UART_Config eUSCI_UART_ConfigV1

// on Launchpad via XDS110

#define USB_UART_PORT GPIO_PORT_P1
#define USB_UART_PINS \
  (GPIO_PIN2 | GPIO_PIN3)  // The pins are given to you for guidance. Also,
                           // because many students miss the parentheses
#define USB_UART_INSTANCE EUSCI_A0_BASE

// An enum outlining what baud rates the UART_construct() function can use in
// its initialization.
enum _UART_Baudrate {
  BAUD_9600,
  BAUD_19200,
  BAUD_38400,
  BAUD_57600,
  NUM_BAUD_CHOICES
};
typedef enum _UART_Baudrate UART_Baudrate;

/*
 * UART Struct:
 *
 * This struct encapsulates parameters necessary for UART communication.
 *
 * Parameters:
 * 1. config: An argument of type UART_Config responsible for configuring UART communication properties,
 *            such as baud rate, parity, stop bits, and data size.
 * 2. moduleInstance: An argument for identifying the specific instance of the UART module to be activated.
 * 3. port: An argument for connecting the program to the corresponding port on the launchpad.
 * 4. pin: An argument for configuring the pins used for UART communication.
 */
struct _UART {
  UART_Config config;

  uint32_t moduleInstance;
  uint32_t port;
  uint32_t pins;
};
typedef struct _UART UART;

// Constructs a uart using the moduleInstance at the given port and pin
UART UART_construct(uint32_t moduleInstance, uint32_t port, uint32_t pins);

// Sets the baudrate and enables a UART based on the given baudrate enum
void UART_SetBaud_Enable(UART*, UART_Baudrate baudrate);


// Returns true if a character is available, false otherwise.
bool UART_hasChar(UART* uart_p);

// Reads and returns a character from the UART buffer.
char UART_getChar(UART* uart_p);

// Checks if the UART module is ready to send data, it returns true if the UART module is ready to send, false otherwise.
bool UART_canSend(UART* uart_p);

// Sends a character over UART.
void UART_sendChar(UART* uart_p, char c);

// Updates the UART baudrate to use the new baud choice.
void UART_updateBaud(UART* uart_p, UART_Baudrate baudChoice);

void UART_Dormant(UART* uart_p);

void UART_reDormant(UART* uart_p);


#endif /* HAL_UART_H_ */
