/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "rtc.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "YS312.h"
#include "debug.h"
#include "dwt_delay.h"
#include "motion.h"
#include "led_beep.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*-----------------------------------------------------------------------------------*/
/** Get time difference between two events
 *
 * \param tickStart Event start time expressed in ticks
 * \param tickEnd	Event stop time expressed in ticks
 * \return 			Difference in ms between two events
 */
__attribute__((always_inline)) inline
uint32_t GetTickDiff(uint32_t tickStart, uint32_t tickEnd)
{
    return (tickEnd - tickStart);
}

/**
 * @brief  Read a single sample from ADC2 IN13 (PA5).
 * @return 12-bit raw value: 0 = 0V, 4095 = 3.3V
 */
uint16_t ADC2_Read(void)
{
    /* Calibrate ADC for better accuracy (optional, run once at startup) */
    // HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

    /* Start single conversion */
    HAL_ADC_Start(&hadc2);

    /* Wait for conversion complete (timeout 10 ms) */
    if (HAL_ADC_PollForConversion(&hadc2, 10) != HAL_OK) {
        HAL_ADC_Stop(&hadc2);
        return 0;
    }

    /* Read raw value */
    uint16_t raw = HAL_ADC_GetValue(&hadc2);

    HAL_ADC_Stop(&hadc2);

    return raw;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	uint16_t reg = 0;
	YS312_Result pir;
	MotionDetector md;
	bool motion = false;


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_USB_Device_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */

  // DWT initialization
  DWT_Delay_Init();

  /* ADC calibration */
  HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

  // Initialize motion detection
  MotionDetector_Init(&md, 500);

  DBG(DBG_DEBUG, "Starting...\n");
  LED_Set();
  Beep(200);
  LED_Clr();

  uint32_t start_time = HAL_GetTick();

  // delay for debug
  //HAL_Delay(5000u);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  /* Poll no faster than tR ≈ 16 ms so the sensor has time to
	       * update its internal data before the next read. */
	      HAL_Delay(200u);

	      // Get regulator value
		  reg = ADC2_Read();
		  MotionDetector_SetThreshold(&md, reg);

		  //DBG(DBG_DEBUG, "Regulator value: %u", reg);

		  pir = YS312_Read();
		  if (!pir.valid) {
			  DBG(DBG_DEBUG, "YS312 read failed\n\r");
			  continue;
		  }

		  motion = MotionDetector_Update(&md, pir.value);
		  if(GetTickDiff(start_time, HAL_GetTick()) > PIR_STARTUP_TIME) {
			  if(motion) {
				  Beep(50);
			  }
		  }

		  DBG(DBG_DEBUG, "PIR : %6d,  base %d: thr: %4d  motion: %s",
				  pir.value, (int16_t)(md.baseline_scaled / 100), md.threshold, motion ? "YES" : "NO");

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
