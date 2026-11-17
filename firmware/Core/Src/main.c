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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd_i2c.h"
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DS18B20_PIN  GPIO_PIN_12
#define DS18B20_PORT GPIOB
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */
void delay_us(uint16_t us);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float adc2_voltage = 0;
float input_voltage = 0;
uint8_t temp_conversion_pending = 0;
uint32_t temp_start_tick = 0;
GPIO_PinState lastButtonState = GPIO_PIN_SET;
GPIO_PinState currentButtonState;

uint32_t lastTempTick = 0;
float temperature = 0;
static void DS18B20_ForceLow(void);
static void DS18B20_ReleaseBus(void);

static inline uint8_t DS18B20_ReadPin(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
}

static void DS18B20_WriteBit(uint8_t bit)
{
    __disable_irq(); // Prevent SysTick from interrupting timing
    DS18B20_ForceLow();

    if (bit)
    {
        delay_us(6);
        DS18B20_ReleaseBus();
        delay_us(64);
    }
    else
    {
        delay_us(60);
        DS18B20_ReleaseBus();
        delay_us(10);
    }
    __enable_irq(); // Re-enable interrupts immediately after
}

static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit;

    __disable_irq(); // Lock the timing window
    DS18B20_ForceLow();
    delay_us(3);

    DS18B20_ReleaseBus();
    delay_us(10); // The sensor holds the data here

    bit = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
    __enable_irq(); // Release the lock

    delay_us(50);
    return bit;
}

static uint8_t DS18B20_Reset(void)
{
    DS18B20_ForceLow();
    delay_us(480);

    DS18B20_ReleaseBus();
    delay_us(80);

    uint8_t presence = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

    delay_us(400);

    return (presence == GPIO_PIN_RESET) ? 0 : 1; // 0 = OK, 1 = FAIL
}

static void DS18B20_WriteByte(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        DS18B20_WriteBit(data & (1 << i));
    }
}

static uint8_t DS18B20_ReadByte(void)
{
    uint8_t value = 0;
    for (int i = 0; i < 8; i++)
    {
        if (DS18B20_ReadBit())
            value |= (1 << i);
    }
    return value;
}

static inline void DS18B20_ForceLow(void)
{
    GPIOB->ODR &= ~GPIO_PIN_12; // fast low
}

static inline void DS18B20_ReleaseBus(void)
{
    GPIOB->ODR |= GPIO_PIN_12;  // release (open drain HIGH)
}

void lcd_safe_update(const char *l1, const char *l2)
{
    lcd_set_cursor(0, 0);
    lcd_send_string(l1);
    lcd_send_string("                "); // overwrite leftovers

    lcd_set_cursor(1, 0);
    lcd_send_string(l2);
    lcd_send_string("                ");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_I2C2_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  // Enable DWT cycle counter for microsecond delay
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  HAL_Delay(200);
  lcd_init();
  lcd_clear();

  lcd_set_cursor(0,0);
  lcd_send_string("Hello!");

  lcd_set_cursor(1,0);
  lcd_send_string("STM32 Ready");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
        // --- Button (robust debounce + edge detect) ---
        static GPIO_PinState stableState = GPIO_PIN_SET;
        static GPIO_PinState lastStableState = GPIO_PIN_SET;
        static uint32_t lastChangeTime = 0;
        GPIO_PinState reading = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        if (reading != stableState) { lastChangeTime = HAL_GetTick(); stableState = reading; }
        if ((HAL_GetTick() - lastChangeTime) > 5)
        {
            if (stableState != lastStableState)
            {
                lastStableState = stableState;
                if (stableState == GPIO_PIN_RESET) HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
            }
        }

        // --- ADC1 read (Current/Amps - Noise Rejection Filter) ---
        #define SAMPLES_TO_SORT 15
        uint32_t adc_samples[SAMPLES_TO_SORT];

        // 1. Capture a burst of samples as fast as possible
        for (int i = 0; i < SAMPLES_TO_SORT; i++)
        {
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 1);
            adc_samples[i] = HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);
        }

        // 2. Simple Bubble Sort to find the median
        for (int i = 0; i < SAMPLES_TO_SORT - 1; i++) {
            for (int j = 0; j < SAMPLES_TO_SORT - i - 1; j++) {
                if (adc_samples[j] > adc_samples[j + 1]) {
                    uint32_t temp = adc_samples[j];
                    adc_samples[j] = adc_samples[j + 1];
                    adc_samples[j + 1] = temp;
                }
            }
        }

        // 3. Throw away the top 4 and bottom 4 samples (the SMPS spikes)
        // Average only the middle 7 samples
        uint32_t middle_sum = 0;
        for (int i = 4; i < 11; i++) {
            middle_sum += adc_samples[i];
        }
        float filtered_adc = middle_sum / 7.0f;

        // 4. Convert to Amps (10x scale)
        float display_amps = (filtered_adc * 3.3f / 4095.0f) * 10.0f;

        // --- ADC2 read (Voltage - Standard Average) ---
        uint32_t sum2 = 0;
        for (int i = 0; i < 16; i++)
        {
            HAL_ADC_Start(&hadc2);
            HAL_ADC_PollForConversion(&hadc2, 1);
            sum2 += HAL_ADC_GetValue(&hadc2);
            HAL_ADC_Stop(&hadc2);
        }
        input_voltage = (sum2 / 16.0f * 3.3f / 4095.0f) * 11.09f;

        // --- Temperature update ---
        if (!temp_conversion_pending)
        {
            if (HAL_GetTick() - lastTempTick >= 1000)
            {
                if (DS18B20_Reset() == 0)
                {
                    DS18B20_WriteByte(0xCC);
                    DS18B20_WriteByte(0x44);
                    temp_conversion_pending = 1;
                    temp_start_tick = HAL_GetTick();
                }
            }
        }
        else if (HAL_GetTick() - temp_start_tick >= 800)
        {
            if (DS18B20_Reset() == 0)
            {
                DS18B20_WriteByte(0xCC);
                DS18B20_WriteByte(0xBE);
                uint8_t temp_l = DS18B20_ReadByte();
                uint8_t temp_h = DS18B20_ReadByte();
                int16_t raw_temp = (temp_h << 8) | temp_l;
                float actual_temp = (float)raw_temp / 16.0f;
                if (actual_temp > -60.0f && actual_temp < 130.0f) temperature = actual_temp;
            }
            temp_conversion_pending = 0;
            lastTempTick = HAL_GetTick();
        }

        // --- Format LCD text ---
        char line1[17], line2[17];
        snprintf(line1, sizeof(line1), "%.2f A %.1f V", display_amps, input_voltage);
        if (temperature < -500) snprintf(line2, sizeof(line2), "T: SENSOR ERROR");
        else snprintf(line2, sizeof(line2), "%.2f C", temperature);

        // --- LCD update every 250ms ---
        static uint32_t lastLcd = 0;
        if (HAL_GetTick() - lastLcd >= 250)
        {
            lcd_safe_update(line1, line2);
            lastLcd = HAL_GetTick();
        }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void delay_us(uint16_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

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
#ifdef USE_FULL_ASSERT
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
