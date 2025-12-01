#ifndef LCD_SHOW_H
#define LCD_SHOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fonts.h"     
#include "ssd1306.h"   


void Savings_Show(void);


void Temperature_Show(float temperature);

#ifdef __cplusplus
}
#endif

#endif 
