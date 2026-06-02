/* motion.c */
#include "motion.h"
#include "stm32g4xx_hal.h"
#include "led_beep.h"
#include "debug.h"

/* ADC 0..4095 > threshold 200..5000 */
#define THRESHOLD_MIN   20
#define THRESHOLD_MAX   2000

void MotionDetector_Init(MotionDetector *md, uint16_t hold_time_ms)
{
    md->threshold        = 500;
    md->detected     	 = false;
    md->baseline_scaled  = 0;
    md->detection_time   = 0;
    md->motion           = false;
}

/**
 * @brief Map potentiometer ADC value to detection threshold.
 *        Call every time ADC is read.
 */
void MotionDetector_SetThreshold(MotionDetector *md, uint16_t adc_raw)
{
    /* Linear map: 0..4095 > THRESHOLD_MIN..THRESHOLD_MAX */
    md->threshold = (int16_t)(THRESHOLD_MIN +
        ((uint32_t)(adc_raw) * (THRESHOLD_MAX - THRESHOLD_MIN)) / 4095u);
}

#define MOTION_DETECT_CNT	5

bool motion_start = false;
uint8_t motion_cnt = 0;

/**
 * @brief Feed new PIR sample, returns true if motion is active.
 *
 * Algorithm:
 *  1. Compute absolute deviation from baseline.
 *  2. If deviation > threshold > trigger, reset hold timer.
 *  3. Motion stays true until hold_time_ms after last trigger.
 *  4. In idle slowly update baseline with low-pass filter.
 */
bool MotionDetector_Update(MotionDetector *md, int16_t pir_value)
{
    /* Get real baseline from scaled value */
    int16_t baseline = (int16_t)(md->baseline_scaled / 100);

    /* Absolute deviation from baseline */
    int16_t deviation = pir_value - baseline;
    if (deviation < 0) deviation = -deviation;

    // Set motion to false before detection
    md->motion = false;

    if (deviation > md->threshold) {
    	if(md->detected) {
    		if (++md->detection_time >= MOTION_DETECT_CNT) {
    	        md->motion = true;
    	        md->detected = false;
    		}
    	} else {
    		md->detected = true;
    		md->detection_time = 0;
    	}
    } else {
    	md->detected = false;
    }

    /* Update baseline only in idle via EMA */
    if (!md->motion && !md->detected) {
        /*
         * baseline_scaled = baseline_scaled * 99/100 + pir_value * 100 * 1/100
         *                 = baseline_scaled * 99/100 + pir_value
         *
         * Multiply pir_value by 100 first to stay in scaled domain,
         * then apply 1/100 weight → just add pir_value directly.
         */
        md->baseline_scaled = md->baseline_scaled * 99 / 100 + pir_value;
    }

    return md->motion;
}


