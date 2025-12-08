#include "DS18B20.h"
#include <math.h>
#include "stm32f4xx_hal.h"
// Configure your data pin here
#define DS18B20_PORT       GPIOA
#define DS18B20_PIN        GPIO_PIN_4
#define DS18B20_PIN_NUM    4U

static void DWT_Delay_Init(void);
static void delay_us(uint32_t us);
static void DS18B20_PinOutput(void);
static void DS18B20_PinInput(void);
static uint8_t OneWire_Reset(void);
static void OneWire_WriteBit(uint8_t bit);
static uint8_t OneWire_ReadBit(void);
static void OneWire_WriteByte(uint8_t data);
static uint8_t OneWire_ReadByte(void);

void DS18B20_Init(void)
{
    /* Enable pull-up and open-drain on the data pin, start released */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    DS18B20_PORT->PUPDR &= ~(3U << (DS18B20_PIN_NUM * 2));
    DS18B20_PORT->PUPDR |=  (1U << (DS18B20_PIN_NUM * 2));
    DS18B20_PORT->OTYPER |= (1U << DS18B20_PIN_NUM);
    DS18B20_PinInput();
    DWT_Delay_Init();
}

float DS18B20_ReadTemperature(void)
{
    uint8_t scratch[9];
    if (!OneWire_Reset()) return NAN;
    OneWire_WriteByte(0xCC); // Skip ROM
    OneWire_WriteByte(0x44); // Convert T
    osDelay(750);            // max conversion time for 12-bit

    if (!OneWire_Reset()) return NAN;
    OneWire_WriteByte(0xCC); // Skip ROM
    OneWire_WriteByte(0xBE); // Read Scratchpad
    for (uint8_t i = 0; i < 9; i++) scratch[i] = OneWire_ReadByte();

    int16_t raw = (scratch[1] << 8) | scratch[0];
    return (float)raw / 16.0f;
}

static void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (SystemCoreClock / 1000000U) * us;
    while ((DWT->CYCCNT - start) < ticks) { }
}

static void DS18B20_PinOutput(void)
{
    DS18B20_PORT->MODER &= ~(3U << (DS18B20_PIN_NUM * 2));
    DS18B20_PORT->MODER |=  (1U << (DS18B20_PIN_NUM * 2)); // output
}

static void DS18B20_PinInput(void)
{
    DS18B20_PORT->MODER &= ~(3U << (DS18B20_PIN_NUM * 2)); // input
}

static uint8_t OneWire_Reset(void)
{
    uint8_t presence;
    DS18B20_PinOutput();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(480);
    DS18B20_PinInput();            // release the line
    delay_us(70);
    presence = (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_RESET);
    delay_us(410);
    return presence;
}

static void OneWire_WriteBit(uint8_t bit)
{
    DS18B20_PinOutput();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us(5);
        DS18B20_PinInput();
        delay_us(60);
    } else {
        delay_us(60);
        DS18B20_PinInput();
        delay_us(5);
    }
}

static uint8_t OneWire_ReadBit(void)
{
    uint8_t bit;
    DS18B20_PinOutput();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    delay_us(3);
    DS18B20_PinInput();
    delay_us(10);
    bit = HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN);
    delay_us(53);
    return bit;
}

static void OneWire_WriteByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        OneWire_WriteBit(data & 0x01);
        data >>= 1;
    }
}

static uint8_t OneWire_ReadByte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;
        if (OneWire_ReadBit()) data |= 0x80;
    }
    return data;
}