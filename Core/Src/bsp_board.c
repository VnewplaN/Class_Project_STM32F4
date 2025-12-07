#include "bsp_board.h"

I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;

void BSP_Init(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM            = 16;
  RCC_OscInitStruct.PLL.PLLN            = 336;
  RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ            = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK   |
                                     RCC_CLOCKTYPE_SYSCLK |
                                     RCC_CLOCKTYPE_PCLK1  |
                                     RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void MX_I2C1_Init(void)
{
  /* Enable GPIOB and I2C1 clocks */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
  (void)RCC->AHB1ENR;
  (void)RCC->APB1ENR;

  /* PB8 = SCL, PB9 = SDA, AF4, open-drain, pull-up, very high speed */
  GPIOB->MODER   &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
  GPIOB->MODER   |=  ((2U << (8 * 2)) | (2U << (9 * 2)));
  GPIOB->OTYPER  |=  ((1U << 8) | (1U << 9));
  GPIOB->OSPEEDR |=  ((3U << (8 * 2)) | (3U << (9 * 2)));
  GPIOB->PUPDR   &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
  GPIOB->PUPDR   |=  ((1U << (8 * 2)) | (1U << (9 * 2)));
  GPIOB->AFR[1]  &= ~((0xFU << ((8 - 8) * 4)) | (0xFU << ((9 - 8) * 4)));
  GPIOB->AFR[1]  |=  ((4U << ((8 - 8) * 4)) | (4U << ((9 - 8) * 4)));

  /* Reset I2C1 */
  RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

  /* Disable before config */
  I2C1->CR1 &= ~I2C_CR1_PE;

  uint32_t pclk1_mhz = 42U;      /* APB1 = 42 MHz */
  uint32_t i2c_speed = 400000U;  /* 400 kHz fast mode */

  I2C1->CR2 &= ~I2C_CR2_FREQ;
  I2C1->CR2 |=  (pclk1_mhz & I2C_CR2_FREQ);

  uint32_t ccr = (pclk1_mhz * 1000000U) / (3U * i2c_speed);
  if (ccr == 0) ccr = 1;
  I2C1->CCR = 0;
  I2C1->CCR |= I2C_CCR_FS;
  I2C1->CCR |= (ccr & I2C_CCR_CCR);

  uint32_t trise = ((pclk1_mhz * 300U) / 1000U) + 1U;
  I2C1->TRISE = (trise & I2C_TRISE_TRISE);

  I2C1->CR1 |= I2C_CR1_ACK;
  I2C1->CR1 |= I2C_CR1_PE;

  hi2c1.Instance             = I2C1;
  hi2c1.Init.ClockSpeed      = 400000U;
  hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2     = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  hi2c1.State                = HAL_I2C_STATE_READY;
  hi2c1.ErrorCode            = HAL_I2C_ERROR_NONE;
}

void MX_USART2_UART_Init(void)
{
  /* Enable GPIOA and USART2 clocks */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  (void)RCC->AHB1ENR;
  (void)RCC->APB1ENR;

  /* PA2/PA3 AF7 (USART2), push-pull, no pull-up/down, medium speed */
  GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
  GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2)));
  GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
  GPIOA->AFR[0] |=  ((7U << (2 * 4)) | (7U << (3 * 4)));
  GPIOA->OTYPER &= ~((1U << 2) | (1U << 3));
  GPIOA->PUPDR  &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
  GPIOA->OSPEEDR |= ((1U << (2 * 2)) | (1U << (3 * 2)));

  uint32_t pclk1 = 42000000UL;
  uint32_t baud  = 115200UL;
  USART2->BRR = (pclk1 + (baud / 2)) / baud;

  USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;
  USART2->CR1 |= USART_CR1_UE;

  huart2.Instance          = USART2;
  huart2.Init.BaudRate     = 115200U;
  huart2.Init.WordLength   = UART_WORDLENGTH_8B;
  huart2.Init.StopBits     = UART_STOPBITS_1;
  huart2.Init.Parity       = UART_PARITY_NONE;
  huart2.Init.Mode         = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.gState            = HAL_UART_STATE_READY;
  huart2.ErrorCode         = HAL_UART_ERROR_NONE;
}

void MX_GPIO_Init(void)
{
  /* Enable GPIOA/B/C clocks */
  RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
  (void)RCC->AHB1ENR;

  /* LED PA5 to low */
  GPIOA->ODR &= ~LD2_Pin;

  const uint32_t ld2_pin_pos = 5U;
  GPIOA->MODER &= ~(3U << (ld2_pin_pos * 2));
  GPIOA->MODER |=  (1U << (ld2_pin_pos * 2));
  GPIOA->OTYPER &= ~(1U << ld2_pin_pos);
  GPIOA->PUPDR  &= ~(3U << (ld2_pin_pos * 2));

  /* PA0, PA1 input with pull-up */
  GPIOA->MODER &= ~((3U << (0 * 2)) | (3U << (1 * 2)));
  GPIOA->PUPDR &= ~((3U << (0 * 2)) | (3U << (1 * 2)));
  GPIOA->PUPDR |=  ((1U << (0 * 2)) | (1U << (1 * 2)));

  /* PC2 MISO, PC3 MOSI AF5 */
  GPIOC->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
  GPIOC->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2)));
  GPIOC->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
  GPIOC->AFR[0] |=  ((5U << (2 * 4)) | (5U << (3 * 4)));

  /* PB10 SCK AF5 */
  GPIOB->MODER &= ~(3U << (10 * 2));
  GPIOB->MODER |=  (2U << (10 * 2));
  GPIOB->AFR[1] &= ~(0xFU << ((10 - 8) * 4));
  GPIOB->AFR[1] |=  (5U << ((10 - 8) * 4));

  /* PB12 CS output high */
  GPIOB->MODER &= ~(3U << (12 * 2));
  GPIOB->MODER |=  (1U << (12 * 2));
  GPIOB->OTYPER &= ~(1U << 12);
  GPIOB->PUPDR  &= ~(3U << (12 * 2));
  GPIOB->BSRR    = GPIO_BSRR_BS12;
}

void MX_SPI2_Init(void)
{
  __HAL_RCC_SPI2_CLK_ENABLE();

  hspi2.Instance               = SPI2;
  hspi2.Init.Mode              = SPI_MODE_MASTER;
  hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi2.Init.NSS               = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64; /* slow for SD */
  hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial     = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}
