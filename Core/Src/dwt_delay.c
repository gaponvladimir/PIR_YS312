#include "main.h"
#include "dwt_delay.h"

/*-----------------------------------------------------------------------------------*/
/**
 * Initialize DWT used as delay counter.
 *
 * \param[in] None
 * \return None
 */
void DWT_Delay_Init(void)
{
	// Check if DWT is not previously enabled
	if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
		// Enable core debug
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		DWT->CYCCNT = 0;
		// Enable DWT counter
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}
