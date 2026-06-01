/*
 * debug.h
 *
 *  Created on: Jul 26, 2024
 *  Author: KVovk
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

#include "main.h"

/** Enable output debug information through UART (serial console) */
//#define DEBUG_VIA_UART

/** Enable output debug information to the JTAG SWO */
#define DEBUG_VIA_JTAG

/** Enable output debug information to the serial console */
#define ENABLE_DEBUG

/** Enable reception some characters from console for start some test */
//#define ENABLE_DEBUG_RECEPTION

/** Length for the buffer used for debug output */
#define DEBUG_TX_BUFFER_SIZE	(unsigned int) 256
/** Size of the RX buffer used for receive characters from console */
#define DEBUG_RX_BUFFER_SIZE	5

/** DEBUG UART is still in sending process if this bit in logic '1' */
#define DEBUG_TX_IN_PROCESS		1
/** DEBUG UART receive something in buffer -> need to process it if this flag is set to logic '1' */
#define DEBUG_RX_COMPLETED		2

/** Dump values to debug output as HEX values */
#define	DUMP_AS_HEX

/** Type of logging levels used for output messages */
typedef enum
{
    DBG_TRACE,     /**< Only for "tracing" the code and find part of a the function that cause errors */
    DBG_DEBUG,     /**< Information that is helpful to people more than just developers (for diagnostic purpose) */
    DBG_INFO,      /**< Generally useful information to log service start/stop, configuration assumptions, etc). */
    DBG_WARNING,   /**< Anything that can potentially cause application oddities, such as retrying an operation, missing secondary data, etc */
    DBG_ERROR,     /**< Any error which is fatal to the operation but not the service or application (can't open a required file, missing data, etc). */
    DBG_FATAL,     /**< Any error that is forcing a shutdown of the service or application to prevent data loss (or further data loss). */
    DBG_DISABLED   /**< Totally disabled the logging output, value used internally, do not use. Not exposed to users. */
} LOG_TYPE;


int  DEBUG_is_enabled(LOG_TYPE log_lvl);
void DEBUG_msg_fmt(const char* fmt, ...);
void DEBUG_msg_fmt_buf(const uint8_t* data, uint32_t size, const char* fmt, ...);
void DEBUG_msg_fmt_buf16(const uint16_t* data, uint32_t size, const char* fmt, ...);
void DEBUG_msg_fmt_buf32(const uint32_t* data, uint32_t size, const char* fmt, ...);

// Macros for output debug information
#ifdef ENABLE_DEBUG
    #define DBG(Level, ...)	            	do {if(DEBUG_is_enabled(Level)) DEBUG_msg_fmt(__VA_ARGS__);} while (0)
    #define DUMP(Level, buf, len, ...)      do {if(DEBUG_is_enabled(Level)) DEBUG_msg_fmt_buf(buf, len,  __VA_ARGS__);} while (0)
	#define DUMP16(Level, buf, len, ...)    do {if(DEBUG_is_enabled(Level)) DEBUG_msg_fmt_buf16(buf, len,  __VA_ARGS__);} while (0)
	#define DUMP32(Level, buf, len, ...)	do {if(DEBUG_is_enabled(Level)) DEBUG_msg_fmt_buf32(buf, len,  __VA_ARGS__);} while (0)
#else
    #define DBG(Level, ...)
    #define DUMP(Level, buf, len, ...)
	#define DUMP16(Level, buf, len, ...)
	#define DUMP32(Level, buf, len, ...)
#endif	// APPLICATION_DEBUG_ENABLE


#ifdef DEBUG_VIA_UART
void DEBUG_TX_Complete_Callback(void);
#endif

// Functionality of the debug reception
#ifdef ENABLE_DEBUG_RECEPTION

extern uint8_t debug_rx_buf[];
extern uint16_t dbg_received;

void DEBUG_RX_Restart(void);
void DEBUG_RX_Callback(uint16_t Size);
void DEBUG_RX_Start(void);
int  DEBUG_RX_Check(void);

#endif // ENABLE_DEBUG_RECEPTION

#endif /* INC_DEBUG_H_ */
