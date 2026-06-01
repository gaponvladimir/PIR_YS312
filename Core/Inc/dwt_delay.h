#ifndef _DWT_DELAY_H
#define _DWT_DELAY_H

#define DWT_DELAY_US(us)                                                  \
    do {                                                                   \
        DWT->CYCCNT = 0;                                                   \
        __DSB();                                                           \
        while (DWT->CYCCNT < ((us) * (SystemCoreClock / 1000000u)));      \
    } while (0)

#define DWT_DELAY_MS(ms)                                                  \
    do {                                                                   \
        DWT->CYCCNT = 0;                                                   \
        __DSB();                                                           \
        while (DWT->CYCCNT < ((us) * (SystemCoreClock / 1000u)));      \
    } while (0)

#define DWT_DELAY(cycles)                                                  \
    do {                                                                   \
        DWT->CYCCNT = 0;                                                   \
        __DSB();                                                           \
        while (DWT->CYCCNT < cycles);      \
    } while (0)


void DWT_Delay_Init(void);

#endif // _DWT_DELAY_H
