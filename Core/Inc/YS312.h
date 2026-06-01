#ifndef YS312_H
#define YS312_H

#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Result of a single YS312 read operation.
 *
 * value - signed 16-bit output of the internal high-pass filter.
 *         Positive/negative swing indicates direction of IR change.
 * valid - true when the frame header (B18=1, B17=0) and
 *         tail (B0=0) pass validation.
 */
typedef struct {
    int16_t value;
    bool    valid;
} YS312_Result;

void         YS312_Init(void);
YS312_Result YS312_Read(void);
YS312_Result YS312_Read_Debug(void);

#endif /* YS312_H */
