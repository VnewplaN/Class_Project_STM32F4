#ifndef DS18B20_H
#define DS18B20_H

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

void DS18B20_Init(void);
float DS18B20_ReadTemperature(void);

#endif /* DS18B20_H */