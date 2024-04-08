/*
 * UART.c
 *
 *  Created on: Dec 31, 2019
 *      Author: Matthew Zhong
 *  Supervisor: Leyla Nazhand-Ali
 */

#include <HAL/Timer.h>
#include <HAL/UART.h>

/**
 * Initializes the UART module except for the baudrate generation
 * Except for baudrate generation, all other uart configuration should match
 * that of basic_example_UART
 *
 * @param moduleInstance:   The eUSCI module used to construct this UART
 * @param port:             The port the UART is connected to
 * @param pins:             The Rx and Tx pins UART is connected to
 *
 * @return the completed UART object
 */
UART UART_construct(uint32_t moduleInstance, uint32_t port, uint32_t pins) {
  // Create the UART struct
  UART uart;

  // Initialize auxiliary members of the struct
  // -------------------------------------------------------------------------
  uart.moduleInstance = moduleInstance;
  uart.port = port;
  uart.pins = pins;

  GPIO_setAsPeripheralModuleFunctionInputPin(uart.port, uart.pins,
                                             GPIO_PRIMARY_MODULE_FUNCTION);


  uart.config.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;  // LSB First

  uart.config.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;  // One stop bit

  uart.config.uartMode = EUSCI_A_UART_MODE;  // UART mode

  uart.config.dataLength = EUSCI_A_UART_8_BIT_LEN;  // Data length is 8 bits

  uart.config.parity = EUSCI_A_UART_NO_PARITY;  // No Parity


  // Return the completed UART instance
  return uart;
}

/**
 * (Re)initializes and (re)enable the UART module to use a desired baudrate.
 *
 * @param uart_p        The pointer to the uart struct that needs a baudrate and
 * should be enabled
 * @param baudChoice:   The new baud choice with which to update the module
 */
void UART_SetBaud_Enable(UART* uart_p, UART_Baudrate baudChoice) {
  // We use the system clock for baudrate generation.
  // The processor clock runs at 48MHz in the project. In other words, system
  // clock is 48MHz.
  uart_p->config.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;

  // Fortunately for us, for all the baudrates we are working with with f =
  // 48MHz, we use oversampling mode N>16
  uart_p->config.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

  // When using the baudChoice to index the array, we are effectively saying
  // that different baudrate choices correspond with different numbers
  // without requiring excessive amounts of if-statements.

  uint32_t clockPrescalerMapping[NUM_BAUD_CHOICES] = {312, 156, 78, 52};
  uint32_t firstModRegMapping[NUM_BAUD_CHOICES] = {8, 4, 2, 1};
  uint32_t secondModRegMapping[NUM_BAUD_CHOICES] = {0, 0, 0, 0x25};

  uart_p->config.clockPrescalar = clockPrescalerMapping[baudChoice];
  uart_p->config.firstModReg = firstModRegMapping[baudChoice];
  uart_p->config.secondModReg = secondModRegMapping[baudChoice];

  UART_initModule(USB_UART_INSTANCE, &uart_p->config);
  UART_enableModule(USB_UART_INSTANCE);
}


/**
 * Determines if the user has sent a UART data packet to the board by checking
 * the interrupt status of the proper system UART module.
 *
 * @param uart_p: The pointer to the UART instance with which to handle our
 * operations.
 *
 * @return true if the user has entered a character, and false otherwise
 */
bool UART_hasChar(UART* uart_p) {
  uint8_t interruptStatus = UART_getInterruptStatus(
      uart_p->moduleInstance, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);

  return (interruptStatus == EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
}

/**
 * Retrieves a character from the UART receive buffer.
 *
 * This function  retrieves and returns the first character
 * from the buffer.
 *
 * @param uart_p A pointer to the UART instance.
 * @return The received character,
 */
char UART_getChar(UART* uart_p) {
    // Read and return the received character
    return UART_receiveData(uart_p->moduleInstance);
}


/**
 * Checks if the UART module can send data.
 *
 * This function checks the interrupt status of the UART module to determine if
 * the transmit buffer is available for sending new data. If the transmit buffer
 * is available (i.e., the transmit interrupt flag is set), it returns true;
 * otherwise, it returns false.
 *
 * @param uart_p A pointer to the UART instance.
 * @return true if UART can send data, false otherwise.
 */
bool UART_canSend(UART* uart_p) {
    // Retrieve the interrupt status of the UART module for transmit interrupt
    uint8_t interruptStatus = UART_getInterruptStatus(
        uart_p->moduleInstance, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);

    // Check if the transmit buffer is available for sending new data
    return (interruptStatus == EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);
}

/**
 * Sends a character over UART.
 *
 * This function sends a single character over UART by writing it to the UART
 * transmit buffer, the character is written to the buffer for
 * transmission.
 *
 * @param uart_p A pointer to the UART instance.
 * @param c The character to be sent over UART.
 */
void UART_sendChar(UART* uart_p, char c) {
    UART_transmitData(uart_p->moduleInstance, (uint_fast8_t)c);
}


void UART_Dormant(UART* uart_p){
    UART_setDormant(uart_p->moduleInstance);
}

void UART_reDormant(UART* uart_p){
    UART_resetDormant(uart_p->moduleInstance);
}
