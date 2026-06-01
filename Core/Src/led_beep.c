#include "main.h"
#include "led_beep.h"
#include "dwt_delay.h"

/*-----------------------------------------------------------------------------------*/
void LED_Set(void)
{
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

/*-----------------------------------------------------------------------------------*/
void LED_Clr(void)
{
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

/*-----------------------------------------------------------------------------------*/
void LED_Toggle(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

/*-----------------------------------------------------------------------------------*/
void Led_Setup_State(GPIO_PinState state)
{
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, state);
}

// sets delay On and after specified delay set OFF (for debug)
void LED_Flash( uint32_t delay )
{
    LED_Set();
    HAL_Delay( delay );
    LED_Clr();
    HAL_Delay( delay );
}

// Blink proper count by proper delay
void LED_Blink(uint32_t cnt, uint32_t delay)
{
    int count = cnt;

    while(count--){
        LED_Flash(delay);
    }
}

void Beep(unsigned int duration)
{
	unsigned int t0 = HAL_GetTick();

	// Set sound duration time
	while(GetTickDiff(t0, HAL_GetTick()) <= duration) {
		HAL_GPIO_WritePin(BUZER_GPIO_Port, BUZER_Pin, GPIO_PIN_SET);
		DWT_DELAY_US(BEEP_PERIOD/2);
		HAL_GPIO_WritePin(BUZER_GPIO_Port, BUZER_Pin, GPIO_PIN_RESET);
		DWT_DELAY_US(BEEP_PERIOD/2);
	}
}
