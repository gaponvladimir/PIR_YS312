/*
 * debug.c
 *
 *  Created on: Jan 23, 2023
 *  Author: KVovk
 */
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "atomic.h"
#include "settings_noprintf.h"
#include "debug.h"

/** Console buffer */
char debug_buf[DEBUG_TX_BUFFER_SIZE];

#ifdef ENABLE_DEBUG_RECEPTION
/** Debug buffer used for reception characters from console */
uint8_t debug_rx_buf[DEBUG_RX_BUFFER_SIZE];

/** Amount of data received by debug UART */
uint16_t dbg_received = 0;
#endif

/** Variable used for inform that transmission from UART completed */
volatile unsigned char debug_uart_status = 0;

/** Default logging configuration for the whole firmware modules */
LOG_TYPE DEBUG_cfg = DBG_TRACE;

#ifdef DEBUG_VIA_JTAG
/** Extern write from sycalls.c overloaded output for printf to SWO */
extern int _write(int fd, char* ptr, int len);
//-----------------------------------------------------------------------------
__STATIC_INLINE uint32_t ITM_is_enabled(void)
{
	/* ITM enabled and ITM Port #0 enabled */
	return (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) && ((ITM->TER & 1UL ) != 0UL));
}

//-----------------------------------------------------------------------------
/** Output one character to SWD
 *
 * \param ch Character to send to SWD
 * \return   The same input value
 */
__STATIC_INLINE int retarget_put_char(int ch)
{
    return (ITM_SendChar((uint32_t)ch));
}

//-----------------------------------------------------------------------------
/** Redefinition of the "putchar" weak function from the syscalls
 *
 * \param ch Character that need to be output at debug console
 * \return   The same input value
 */
int __io_putchar(int ch)
{
	// Send character to the SWD
	return (retarget_put_char(ch));
}
#endif

//-----------------------------------------------------------------------------
/** Checks whether the logging output is enabled for given module and log level.
 *
 * \param module Module for which we check status
 * \param log_lvl Provided logging level
 * \return Nonzero value if logging is enabled for provided module and logging level is less the provided in module as default
 */
int DEBUG_is_enabled(LOG_TYPE log_lvl)
{
    return (log_lvl >= DEBUG_cfg);
}

//-----------------------------------------------------------------------------
/** Check if DEBUG UART is ready to send next debug message
 *
 * \return '0' - is ready to send new message, '-1' - otherwise
 */
int DEBUG_ready_for_tx(void)
{
	// If we busy now with output something by DMA on UART
	if (debug_uart_status & DEBUG_TX_IN_PROCESS) {
		// Return "-1"
		return (-1);
	}
	else {
		ATOMIC() {
			// Set that we will busy with output string by DEBUG UART
			debug_uart_status |= DEBUG_TX_IN_PROCESS;
		}
		// Return "0"
		return (0);
	}
}

//-----------------------------------------------------------------------------
/** Output debug message to the specified debug interface
 *
 * Function declares internal console buffer \sa DEBUG_TX_BUFFER_SIZE where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param fmt Format string that is used for format message
 * \param ... Text message and message data
 * \return None
 */
void DEBUG_msg_fmt(const char* fmt, ...)
{
    va_list arglist;
    int pos;

#ifdef DEBUG_VIA_UART
    // Wait for UART is ready for start new transmission
    while (DEBUG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = cg_vsnprintf(&debug_buf[0], DEBUG_TX_BUFFER_SIZE, fmt, arglist);

    va_end(arglist);

    // Check if we have buffer overflow
    if (pos >= DEBUG_TX_BUFFER_SIZE - 3) {
    	// Truncate and output CR+LF at the end of string
    	cg_snprintf(&debug_buf[DEBUG_TX_BUFFER_SIZE - 3], 3, "\r\n");
        pos = DEBUG_TX_BUFFER_SIZE - 1;
    }
    // Output CR+LF at the end of string
    else {
    	pos += cg_snprintf(&debug_buf[pos], 3, "\r\n");
    }
#ifdef DEBUG_VIA_JTAG
    // Check if JTAG is connected
    if (ITM_is_enabled()) {        // Output to SWV only if it is connected
    	_write(0, debug_buf, pos); // output user message. There is no reason to call printf, since everything has already been formated.
    }
#endif

#ifdef DEBUG_VIA_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
    ;
    // Output data to UART
    HAL_UART_Transmit_DMA(&DEBUG_UART, (uint8_t *) debug_buf, pos);
#endif
}

//-----------------------------------------------------------------------------
/** Output debug message and data in HEX representation to the specified debug interface.
 *
 * Function declares internal console buffer \sa DEBUG_TX_BUFFER_SIZE where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param data Pointer to the byte data buffer
 * \param size Size of the data buffer
 * \param fmt Format string that is used for format message
 * \param ... Message and message data
 * \return None
 */
void DEBUG_msg_fmt_buf(const uint8_t* data, uint32_t size, const char* fmt, ...)
{
    va_list arglist;
    int pos;
    uint32_t i = 0;

#ifdef DEBUG_VIA_UART
    // Wait for UART is ready for start new transmission
    while (DEBUG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = cg_vsnprintf(&debug_buf[0], DEBUG_TX_BUFFER_SIZE, fmt, arglist);

    va_end(arglist);

    // If we have a space in console buffer
    while ((pos < DEBUG_TX_BUFFER_SIZE) && (i < size)) {
    	// Output supplied array as HEX characters with spaces
        pos += cg_snprintf(&debug_buf[pos], (DEBUG_TX_BUFFER_SIZE - pos), "%02X ", data[i++]);
    }

    if (pos >= (DEBUG_TX_BUFFER_SIZE - 3)) {
        cg_snprintf(&debug_buf[DEBUG_TX_BUFFER_SIZE-3], 3, "\r\n");
        pos = DEBUG_TX_BUFFER_SIZE - 1;
    }
    else {
        pos += cg_snprintf(&debug_buf[pos], DEBUG_TX_BUFFER_SIZE - pos, "\r\n");
    }

#ifdef DEBUG_VIA_JTAG
    if (ITM_is_enabled()) {
        _write(0, debug_buf, pos); // output user message. There is no reason to call printf, since everything has already been formated.
    }
#endif

#ifdef DEBUG_VIA_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
	;
    // Output data to UART
    HAL_UART_Transmit_DMA(&DEBUG_UART, (uint8_t *) debug_buf, pos);
#endif
}


//-----------------------------------------------------------------------------
/** Output debug message and data in HEX representation to the specified debug interface.
 *
 * Function declares internal console buffer \sa DEBUG_TX_BUFFER_SIZE where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param data Pointer to the short data buffer
 * \param size Size of the data buffer
 * \param fmt Format string that is used for format message
 * \param ... Message and message data
 * \return None
 */
void DEBUG_msg_fmt_buf16(const uint16_t* data, uint32_t size, const char* fmt, ...)
{
    va_list arglist;
    int pos;
    uint32_t i = 0;

#ifdef DEBUG_VIA_UART
    // Wait for UART is ready for start new transmission
    while (DEBUG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = cg_vsnprintf(&debug_buf[0], DEBUG_TX_BUFFER_SIZE, fmt, arglist);

    va_end(arglist);


    // If we have a space in console buffer
    while ((pos < DEBUG_TX_BUFFER_SIZE) && (i < size)) {
    	// Output supplied array as HEX characters with spaces
        pos += cg_snprintf(&debug_buf[pos], (DEBUG_TX_BUFFER_SIZE - pos), "%04X ", data[i++]);
    }

    if (pos >= (DEBUG_TX_BUFFER_SIZE - 3)) {
        cg_snprintf(&debug_buf[DEBUG_TX_BUFFER_SIZE-3], 3, "\r\n");
        pos = DEBUG_TX_BUFFER_SIZE - 1;
    }
    else {
        pos += cg_snprintf(&debug_buf[pos], DEBUG_TX_BUFFER_SIZE - pos, "\r\n");
    }

#ifdef DEBUG_VIA_JTAG
    if (ITM_is_enabled()) {
        _write(0, debug_buf, pos); // output user message. There is no reason to call printf, since everything has already been formated.
    }
#endif

#ifdef DEBUG_VIA_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
    ;
    // Output data to UART
    HAL_UART_Transmit_DMA(&DEBUG_UART, (uint8_t *) debug_buf, pos);
#endif
}

//-----------------------------------------------------------------------------
/** Output debug message and data in HEX representation to the specified debug interface.
 *
 * Function declares internal console buffer \sa DEBUG_TX_BUFFER_SIZE where all debug data
 * will be stored. At the end this console buffer will be output to the specified
 * debug interface (UART, JTAG, ..). At the end of each string CR+LF is added.
 * If output message not fit to the declared console buffer it will be truncated
 * and the CR+LF characters will be appended to the end of the truncated string.
 *
 * \param data Pointer to the word data buffer
 * \param size Size of the data buffer
 * \param fmt Format string that is used for format message
 * \param ... Message and message data
 * \return None
 */
void DEBUG_msg_fmt_buf32(const uint32_t* data, uint32_t size, const char* fmt, ...)
{
    va_list arglist;
    int pos;
    uint32_t i = 0;

#ifdef DEBUG_UART
    // Wait for UART is ready for start new transmission
    while (DEBUG_ready_for_tx() < 0);
#endif

    va_start(arglist, fmt);

    pos = cg_vsnprintf(&debug_buf[0], DEBUG_TX_BUFFER_SIZE, fmt, arglist);

    va_end(arglist);


    // If we have a space in console buffer
    while ((pos < DEBUG_TX_BUFFER_SIZE) && (i < size)) {
#ifdef DUMP_AS_HEX
    	// Output supplied array as HEX characters with spaces
        pos += cg_snprintf(&debug_buf[pos], (DEBUG_TX_BUFFER_SIZE - pos), "%08lX ", data[i++]);
#else
    	pos += cg_snprintf(&debug_buf[pos], (DEBUG_TX_BUFFER_SIZE - pos), "%08ld ", data[i++]);
#endif
    }

    if (pos >= (DEBUG_TX_BUFFER_SIZE - 3)) {
        cg_snprintf(&debug_buf[DEBUG_TX_BUFFER_SIZE-3], 3, "\r\n");
        pos = DEBUG_TX_BUFFER_SIZE - 1;
    }
    else {
        pos += cg_snprintf(&debug_buf[pos], DEBUG_TX_BUFFER_SIZE - pos, "\r\n");
    }

#ifdef DEBUG_VIA_JTAG
    if (ITM_is_enabled()) {
        _write(0, debug_buf, pos); // output user message. There is no reason to call printf, since everything has already been formated.
    }
#endif

#ifdef DEBUG_VIA_UART
	// Now DEBUG control logic disallow application to put MCU into the STOP mode
    ;
    // Output data to UART
    HAL_UART_Transmit_DMA(&DEBUG_UART, (uint8_t *) debug_buf, pos);
#endif
}

//-----------------------------------------------------------------------------
/** Callback for TX complete event
 *
 * This callback is called when all data transfered by the debug UART
 */
void DEBUG_TX_Complete_Callback(void)
{
	// Reset flag that UART still in the transmission process
	debug_uart_status &= (~DEBUG_TX_IN_PROCESS);

	// Now DEBUG control logic allow application to put MCU into the STOP mode
	;
}

#ifdef ENABLE_DEBUG_RECEPTION
//-----------------------------------------------------------------------------
/** Function that restart reception data from debug into internal buffer
 *
 * It enables reception only if the flag 'DEBUG_RX_COMPLETED' was set (interrupt was fired and some data was already in buffer).
 *
 * \return	None
 */
void DEBUG_RX_Restart(void)
{
	// Check if something received from DEBUG console
	if (DEBUG_RX_Check()) {
		// Rest flag that command received by DEBUG console
		debug_uart_status &= (~DEBUG_RX_COMPLETED);

		// Clear amount of received data
		dbg_received = 0;

		// Re-enable reception data from PC console
		DEBUG_RX_Start();
	}
}

//-----------------------------------------------------------------------------
/** Callback that is used for process received data from DEBUG console
 *
 * At this moment we support only IDLE event. So amount of data send by console must be less then reception buffer.
 * Function set \sa DBG_RX_COMPLETED flag in the \sa debug_uart_status variable. Reception must be started again
 * in the processing function.
 * Maybe reception must be started after buffer processing or we need use double-buffering for not miss something.
 * This function is called from interrupt context.
 *
 * \param	Size	Amount of data received in buffer
 * \return None
 */
void DEBUG_RX_Callback(uint16_t Size)
{
	// If this IDLE event or reception buffer is full
	if ((DEBUG_UART.RxEventType == HAL_UART_RXEVENT_IDLE) || (DEBUG_UART.RxEventType == HAL_UART_RXEVENT_TC)) {
		// Abort reception -> we already receive something
		HAL_UART_AbortReceive_IT(&DEBUG_UART);
		// Inform application that something received from debug console
		debug_uart_status |= DEBUG_RX_COMPLETED;

		// Save amount of received data located now in RX buffer
		dbg_received = Size;
	}
}

//-----------------------------------------------------------------------------
/** Start reception something from remote debug console
 *
 * Function will start reception by the RX interrupt into the \sa debug_rx_buf.
 * Reception will be end if the buffer completely filled or IDLE event detected on the line
 */
void DEBUG_RX_Start(void)
{
	HAL_StatusTypeDef status;

	status = HAL_UARTEx_ReceiveToIdle_IT(&DEBUG_UART, (uint8_t *) debug_rx_buf, sizeof(debug_rx_buf));
	// If something is wrong (BUSY or ERROR) in enable new reception
	if (status != HAL_OK) {
		// Abort previous reception
		HAL_UART_AbortReceive_IT(&DEBUG_UART);
		// Start reception again
		HAL_UARTEx_ReceiveToIdle_IT(&DEBUG_UART, (uint8_t *) debug_rx_buf, sizeof(debug_rx_buf));
	}
/*
	else {
		DBG(DBG_TRACE, "Debug RX started");
	}
*/
}

//-----------------------------------------------------------------------------
/** Check if something received by debug UART
 *
 * \return	'0' - nothing received, '1' - Received
 */
int DEBUG_RX_Check(void)
{
	return (debug_uart_status & DEBUG_RX_COMPLETED);
}

#endif // ENABLE_DEBUG_RECEPTION
