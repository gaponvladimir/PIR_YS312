/* motion.h */
#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>
#include <stdbool.h>

#define MOTION_THRESHOLD_DEFAULT		100

typedef struct {
    int16_t  threshold;
    int32_t  baseline_scaled; /* baseline × 100 to keep precision */
    bool     motion;
    bool     detected;
    uint32_t detection_time;
} MotionDetector;

void MotionDetector_Init(MotionDetector *md, uint16_t hold_time_ms);
void MotionDetector_SetThreshold(MotionDetector *md, uint16_t adc_raw);
bool MotionDetector_Update(MotionDetector *md, int16_t pir_value);
void Beep(unsigned int duration);

#endif
