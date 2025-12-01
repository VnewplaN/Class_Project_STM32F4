#include "lcd_show.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>


void Savings_Show(void)
{
    SSD1306_Clear();
    SSD1306_GotoXY(1, 0);
    SSD1306_Puts("The system is in", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(1, 16);
    SSD1306_Puts("SAVING MODE", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}

void Temperature_Show(float temperature)
{
    char buf[20];

    SSD1306_Clear();

    SSD1306_GotoXY(1, 0);
    SSD1306_Puts("TEMP:", &Font_7x10, SSD1306_COLOR_WHITE);
    
    // Round to one decimal place
    int tscaled = (int)(temperature * 10.0f + (temperature >= 0 ? 0.5f : -0.5f));
    int whole = tscaled / 10;
    int frac = abs(tscaled % 10);
    snprintf(buf, sizeof(buf), "%d.%d C", whole, frac);

    SSD1306_GotoXY(1, 20);
    SSD1306_Puts(buf, &Font_11x18, SSD1306_COLOR_WHITE);

    SSD1306_UpdateScreen();
}

