#ifndef _LED_BEEP_H
#define _LED_BEEP_H

#define BEEP_PERIOD	1000

void LED_Set(void);
void LED_Clr(void);
void LED_Toggle(void);
void Led_Setup_State(GPIO_PinState state);
void LED_Flash( uint32_t delay );
void LED_Blink(uint32_t cnt, uint32_t delay);

void Beep(unsigned int duration);

#endif
