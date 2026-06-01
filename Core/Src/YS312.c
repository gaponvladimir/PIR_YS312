/* ys312.c
 *
 * Driver for the SENBA YS312 single-wire PIR sensor (DOCI protocol).
 *
 * Wire:  pin 1 = VDD (2.2 V – 5.5 V)
 *        pin 2 = DOCI  →  PB10
 *        pin 3 = VSS
 *
 * Protocol summary (19 bits, MSB first):
 *   B18      – header, always 1
 *   B17      – header, always 0
 *   B16..B1  – 16-bit signed HPF value (two's complement)
 *   B0       – tail,   always 0
 *
 * Timing (from datasheet):
 *   tS  100 µs – 5 ms   starting signal (MCU holds line HIGH)
 *   tL  4 – 8 µs        pull-down pulse per bit
 *   tH  4 – 8 µs        pull-up  pulse per bit
 *   tB  4 – 8 µs        wait for data stabilisation before sampling
 *   tR  ≈ 16 ms         sensor internal data update period
 */

#include "ys312.h"
#include "debug.h"
#include "critical_section.h"
#include "dwt_delay.h"
#include "stdio.h"

/* ------------------------------------------------------------------ */
/*  Timing constants (microseconds)                                   */
/* ------------------------------------------------------------------ */
#define T_S_US   	150u   /* Starting signal HIGH duration 							 */
#define T_L_US     	6u   	/* Pull-down time per bit                                    */
#define T_H_US     	6u   	/* Pull-up  time per bit                                     */
#define T_B_US     	6u   	/* Data stabilization wait before sampling                   */

#define YS312_IDLE_TIME		16	// Max DOCI pin IDLE time

/* ------------------------------------------------------------------ */
/*  Pin direction helpers                                               */
/*                                                                      */
/*  IMPORTANT (datasheet note):                                         */
/*    Always set the output level BEFORE switching to output mode.      */
/*    Switching mode first causes a brief unwanted glitch on the line.  */
/* ------------------------------------------------------------------ */

/**
 * @brief Drive DOCI line HIGH (output push-pull).
 */
static inline void _pin_drive_high(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. Pre-load the output register while still in any mode */
	HAL_GPIO_WritePin(YS312_GPIO_Port, YS312_Pin, GPIO_PIN_SET);

    /* 2. Now switch to output – line stays HIGH without a glitch */
	GPIO_InitStruct.Pin = YS312_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStruct.Pull = GPIO_NOPULL;
	//GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(YS312_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief Drive DOCI line LOW (output push-pull).
 */
static inline void _pin_drive_low(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. Pre-load LOW before switching to output */
    HAL_GPIO_WritePin(YS312_GPIO_Port, YS312_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = YS312_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStruct.Pull = GPIO_NOPULL;
	//GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(YS312_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief Release DOCI line: switch to floating input (no pull resistors).
 *
 * "Release" per datasheet = MCU switches to input mode with no pull-up/down.
 * In idle state the sensor itself holds the line LOW.
 * After data output the sensor pulls the line HIGH.
 */
static inline void _pin_release(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = YS312_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	//GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(YS312_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief Read current logic level of the DOCI line.
 */
static inline GPIO_PinState _pin_read(void)
{
    return HAL_GPIO_ReadPin(YS312_GPIO_Port, YS312_Pin);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void print_bin(uint32_t val)
{
	for(int i = 31; i >= 0; i--) {
		printf("%d", (unsigned int)(val >> i) & 1);
	}
	printf("\n");
}

/**
 * @brief Perform one full 19-bit read cycle from the YS312 sensor.
 *
 * Step-by-step per datasheet Figure 1:
 *
 *  ❶ Wait for the sensor to pull the line HIGH (data ready).
 *     Timeout = 20 ms. If the line has been idle > 16 ms the MCU
 *     briefly drives HIGH then releases it to nudge the sensor.
 *
 *  ❷ Hold line HIGH for tS = 1 ms (within the 100 µs – 5 ms window).
 *     Keep tS constant between reads to avoid data anomalies.
 *
 *  ❸ – ❻  For each of the 19 bits (MSB first):
 *       ❸ MCU pulls line LOW   for tL = 6 µs
 *       ❹ MCU drives line HIGH for tH = 6 µs
 *          Release the line – sensor now drives its data bit
 *       ❺ Wait tB = 6 µs for level to stabilise
 *       ❻ Sample: HIGH → bit = 1 / LOW → bit = 0
 *
 *  ❽ End of frame: MCU pulls LOW then releases.
 *
 *  ❾ Sensor begins next update cycle (≈ 16 ms).
 *
 * @return YS312_Result
 *         .valid = true  → frame header/tail OK, .value is usable
 *         .valid = false → sensor timeout or framing error
 */
YS312_Result YS312_Read(void)
{
    YS312_Result result = { .value = 0, .valid = false };
    uint32_t     raw    = 0u;
    int8_t bit;


    /* ── ❶  Wait for sensor to assert line HIGH ─────────────────── */
    uint32_t t0 = HAL_GetTick();
    while (_pin_read() == GPIO_PIN_RESET) {
    	if(GetTickDiff(t0, HAL_GetTick()) > YS312_IDLE_TIME) {
        	DBG(DBG_DEBUG, "TIMEOUT waiting for HIGH\n");

            /*
             * Line has been LOW for > 20 ms.
             * Datasheet: if idle > 16 ms, MCU should raise the line
             * and release it to prompt a new data cycle.
             */
            _pin_drive_high();
            DWT_DELAY_US(10u);		/* short pulse, then release */
            _pin_release();

            /* Give the sensor another 20 ms to respond */
            t0 = HAL_GetTick();
            while (_pin_read() == GPIO_PIN_RESET) {
            	if(GetTickDiff(t0, HAL_GetTick()) > YS312_IDLE_TIME) {
                	DBG(DBG_DEBUG, "sensor not responding");
                    return result;   /* sensor not responding */
                }
            }
            break;
        }
    }

    /* Line is now HIGH – sensor has fresh data ready */
    /* ── ❷  Hold HIGH for tS (1 ms) ────────────────────────────── */
    DWT_DELAY_US(T_S_US);	/* 100 µs – 5 ms window */

    // Disable interrupts
    CPU_INIT_CRITICAL_SECTION();
    CPU_ENTER_CRITICAL_SECTION();

    /* ── ❸❹❺❻❼  Read 19 bits, MSB (B18) first ──────────────────── */
    for(bit = 0; bit < 19; bit++) {

        /* ❸ Pull LOW for tL */
        _pin_drive_low();
        DWT_DELAY_US(T_L_US);

        /* ❹ Drive HIGH for tH, then release so sensor can drive */
        _pin_drive_high();
        DWT_DELAY_US(T_H_US);

        _pin_release();

        /* ❺ Wait tB for the sensor output to stabilise */
        DWT_DELAY_US(T_B_US);

        // Shift value to left by 1
        raw <<= 1u;

        /* ❻ Sample the bit */
        if (_pin_read() == GPIO_PIN_SET) {
            raw |= 1u;
        }
    }

    /* ── ❽  End-of-frame: pull LOW, then release ────────────────── */
    _pin_drive_low();
    DWT_DELAY_US(T_L_US);
    _pin_release();

    // Enable interrupts
    CPU_EXIT_CRITICAL_SECTION();

    /* ── Validate frame header and tail ─────────────────────────── */
    /*
     * Expected frame layout:
     *   B18 = 1  (header bit, always HIGH)
     *   B17 = 0  (header bit, always LOW)
     *   B16..B1  16-bit signed sensor value
     *   B0  = 0  (tail bit, always LOW)
     */
    bool b18_ok = (((raw >> 18u) & 1u) == 1u);
    bool b17_ok = (((raw >> 17u) & 1u) == 0u);
    bool b0_ok  = (((raw >>  0u) & 1u) == 0u);

    if (b18_ok && b17_ok && b0_ok) {
        /*
         * Extract bits B16..B1 and reinterpret as a signed 16-bit value.
         * The sensor outputs two's complement, so a direct cast is correct.
         */
        uint16_t raw16  = (uint16_t)((raw >> 1u) & 0xFFFFu);
        result.value    = (int16_t)raw16;
        result.valid    = true;
    } else {
    	DBG(DBG_DEBUG, "header/tail is wrong: %d%d%d", (raw >> 18u) & 1u, (raw >> 17u) & 1u, (raw >> 0u) & 1u);
    }

    //print_bin(raw);

    return result;
}

