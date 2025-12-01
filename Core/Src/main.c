/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>
#include "LCD_Show.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
enum {
  Savings_Mode,
  Working_Mode,
};
int state = Savings_Mode; // state of the system
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

osThreadId_t defaultTaskHandle;
osThreadId_t buttonTaskHandle;

const osThreadAttr_t buttonTask_attributes = {
  .name = "buttonTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,      // Increase for SSD1306 + FreeRTOS
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void *argument);
void StartButtonTask(void *argument);
void System_Init(void);
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  System_Init();
  osKernelInitialize();
  buttonTaskHandle = osThreadNew(StartButtonTask, NULL, &buttonTask_attributes);
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  osKernelStart();

  while (1)
  {
  }
}


void System_Init(void)
{
  // Initialize system components if needed
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();
    SSD1306_Init();
}
/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    Error_Handler();
}

/**
  * @brief I2C1 Initialization Function
  */
static void MX_I2C1_Init(void)
{
    /* CMSIS register-level version of I2C1 initialization for STM32F401
     * - Configure PB8/PB9 AF4, open-drain, pull-up
     * - Enable I2C1 peripheral clock
     * - Configure CR2, CCR, TRISE for 400 kHz
     * - Enable peripheral
     */

    /* Enable GPIOB and I2C1 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   // GPIOB
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;    // I2C1
    (void)RCC->AHB1ENR; (void)RCC->APB1ENR;

    /* Configure PB8/PB9 as AF4, open-drain, pull-up, very high speed */
    /* Set mode to AF (10) for PB8 & PB9 */
    GPIOB->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->MODER |=  ((2U << (8 * 2)) | (2U << (9 * 2)));
    /* Set output type to open-drain */
    GPIOB->OTYPER |= ((1U << 8) | (1U << 9));
    /* Very high speed */
    GPIOB->OSPEEDR |= ((3U << (8 * 2)) | (3U << (9 * 2)));
    /* Pull-up */
    GPIOB->PUPDR &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->PUPDR |=  ((1U << (8 * 2)) | (1U << (9 * 2)));
    /* AF4 (I2C1) for PB8/PB9 (use AFR[1] for pins 8..15) */
    GPIOB->AFR[1] &= ~((0xFU << ((8 - 8) * 4)) | (0xFU << ((9 - 8) * 4)));
    GPIOB->AFR[1] |=  ((4U << ((8 - 8) * 4)) | (4U << ((9 - 8) * 4)));

    /* Reset I2C1 to ensure clean config then release reset */
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    /* Disable peripheral before configuring */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* Configure CR2: APB1 frequency in MHz (PCLK1 = 42 MHz in current clock
     * configuration). We set CR2 FREQ field with that value. */
    uint32_t pclk1_mhz = 42U;
    I2C1->CR2 &= ~I2C_CR2_FREQ;
    I2C1->CR2 |= pclk1_mhz & I2C_CR2_FREQ;

    /* Fast mode 400 kHz, duty cycle = 0 (tLow/tHigh = 2) */
    uint32_t i2c_speed = 400000U; // 400 kHz
    uint32_t ccr = (pclk1_mhz * 1000000U) / (3U * i2c_speed);
    if (ccr == 0) ccr = 1;
    I2C1->CCR = 0;
    I2C1->CCR |= (1U << 15); /* F/S = 1 (fast mode) */
    /* Leave DUTY = 0 */
    I2C1->CCR |= (ccr & I2C_CCR_CCR);

    /* TRISE: For fast mode, approx = (PCLK1_MHz * 300ns / 1ns) + 1 */
    uint32_t trise = ((pclk1_mhz * 300U) / 1000U) + 1U;
    I2C1->TRISE = (trise & I2C_TRISE_TRISE);

    /* Enable ACK and the peripheral */
    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR1 |= I2C_CR1_PE;

    /* Initialize HAL handle fields so higher-level HAL APIs can still be used */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000U;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    hi2c1.State = HAL_I2C_STATE_READY;
    hi2c1.ErrorCode = HAL_I2C_ERROR_NONE;
}

/**
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
    /* CMSIS register-level initialization for USART2 (PA2 = TX, PA3 = RX)
     * Basic config: 115200, 8N1, enable TE/RE and the peripheral
     */

    /* Enable GPIOA and USART2 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->AHB1ENR; (void)RCC->APB1ENR;

    /* Configure PA2/PA3 AF7 (USART2), push-pull, no pull-up/dn, medium speed */
    GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2)));
    /* AF7 */
    GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
    GPIOA->AFR[0] |=  ((7U << (2 * 4)) | (7U << (3 * 4)));
    /* Push-pull */
    GPIOA->OTYPER &= ~((1U << 2) | (1U << 3));
    /* No pull */
    GPIOA->PUPDR &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    /* Medium speed */
    GPIOA->OSPEEDR |= ((1U << (2 * 2)) | (1U << (3 * 2)));

    /* Configure USART2 baud rate -> BRR = PCLK1 / baud, APB1 = 42MHz */
    uint32_t pclk1 = 42000000UL;
    uint32_t baud = 115200UL;
    USART2->BRR = (pclk1 + (baud / 2)) / baud;

    /* Enable TX and RX and USART */
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART2->CR1 |= USART_CR1_UE;

    /* Keep HAL handle consistent to allow use of HAL UART APIs */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200U;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.gState = HAL_UART_STATE_READY;
    huart2.ErrorCode = HAL_UART_ERROR_NONE;
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
    /* Configure GPIO pins with CMSIS (LD2/LED on PA5)
     * Keep enabling all GPIO clocks to match previous behavior
     */
    /* Enable GPIOA/B/C clocks */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
    (void)RCC->AHB1ENR;

    /* Reset LED pin (PA5) to low */
    GPIOA->ODR &= ~LD2_Pin;

    /* Configure PA5 as general purpose output push-pull, no pull, low speed */
    /* MODER: 01 = general purpose output. LD2_Pin is a bitmask, so we calculate
     * the pin position directly. */
    const uint32_t ld2_pin_pos = 5U; /* LD2 = PA5 */
    GPIOA->MODER &= ~(3U << (ld2_pin_pos * 2));
    GPIOA->MODER |=  (1U << (ld2_pin_pos * 2));
    /* Output type: push-pull */
    GPIOA->OTYPER &= ~(1U << ld2_pin_pos);
    /* No pull */
    GPIOA->PUPDR &= ~(3U << (ld2_pin_pos * 2));
    /* Low speed (00) -> leave OSPEEDR default (reset 00) */
    //Set PA0 as input for power button
    GPIOA->MODER &= ~(3 << (0 * 2)); //00 for input
    GPIOA->PUPDR |= (1<<(0*2)); //pull-up
    //Set PA1 as input for working mode button
    GPIOA->MODER &= ~(3 << (1 * 2)); //00 for
    GPIOA->PUPDR |= (1<<(1*2)); //pull-up
}

/**
  * @brief  Function implementing the defaultTask thread.
  */
void StartDefaultTask(void *argument)
{
  int last_state = -1;
    for(;;)
    {
      if(state != last_state){
        if(state == Savings_Mode)
        {
            Savings_Show();
        }
        else if(state == Working_Mode)
        {
            float temperature = 25.3f;
            Temperature_Show(temperature); 
        }
      last_state = state;
    }

        osDelay(200); 
    }
}

/**
  * @brief  Function implementing the button thread.
  */
void StartButtonTask(void *argument){
  for(;;)
  {
    if ((GPIOA->IDR & (1<<0)) == 0) { 
        state = Savings_Mode;
    }
    else if ((GPIOA->IDR & (1<<1)) == 0){
        state = Working_Mode;
    }

    osDelay(20); // debounce
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
