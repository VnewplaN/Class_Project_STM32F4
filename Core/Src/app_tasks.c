#include "app_tasks.h"

#include "bsp_board.h"
#include "cmsis_os.h"
#include "DS18B20.h"
#include "File_handling_RTOS.h"
#include "LCD_Show.h"
#include "fatfs.h"
#include <stdio.h>

enum {
  Savings_Mode = 0,
  Working_Mode,
};

static TaskHandle_t defaultTaskHandle;
static TaskHandle_t buttonTaskHandle;
static TaskHandle_t sdcardTaskHandle;

static volatile int state         = Savings_Mode;
static volatile int g_log_enabled = 0;
static volatile int g_temperature = 0;

static void format_temperature(char *buf, size_t size, int temp)
{
  snprintf(buf, size, "%d", temp);
}

static void StartDefaultTask(void *argument);
static void StartButtonTask(void *argument);
static void StartSdcardTask(void *argument);

void App_CreateTasks(void)
{
  xTaskCreate(StartButtonTask,
              "buttonTask",
              256,
              NULL,
              2,
              &buttonTaskHandle);

  xTaskCreate(StartDefaultTask,
              "defaultTask",
              512,
              NULL,
              2,
              &defaultTaskHandle);

  xTaskCreate(StartSdcardTask,
              "sdcardTask",
              512,
              NULL,
              1,
              &sdcardTaskHandle);
}

static void StartDefaultTask(void *argument)
{
  int last_state = -1;
  uint32_t last_measure_tick = 0;

  for (;;)
  {
    if (state != last_state)
    {
      if (state == Savings_Mode)
      {
        Savings_Show();
      }
      else
      {
        float t = DS18B20_ReadTemperature();
        g_temperature = (int)(t >= 0.0f ? t + 0.5f : t - 0.5f);
        Temperature_Show(g_temperature);
        last_measure_tick = xTaskGetTickCount();
      }
      last_state = state;
    }

    if (state == Working_Mode)
    {
      uint32_t now = xTaskGetTickCount();
      if ((now - last_measure_tick) >= pdMS_TO_TICKS(10000))
      {
        last_measure_tick = now;
        float t = DS18B20_ReadTemperature();
        g_temperature = (int)(t >= 0.0f ? t + 0.5f : t - 0.5f);
        Temperature_Show(g_temperature);
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(200));
    }
  }
}

static void StartButtonTask(void *argument)
{
  for (;;)
  {
    uint32_t idr = GPIOA->IDR;

    /* POWER BUTTON (PA0) */
    if (!(idr & (1U << 0)))
    {
      if (state == Savings_Mode)
      {
        state = Working_Mode;
        g_log_enabled = 0;   /* logging off when just powered on */
      }
      else
      {
        state = Savings_Mode;
        g_log_enabled = 0;   /* stop logging when turning off */
      }
      vTaskDelay(pdMS_TO_TICKS(300)); /* debounce */
    }

    /* START BUTTON (PA1) */
    else if (!(idr & (1U << 1)))
    {
      if (state == Working_Mode)
      {
        g_log_enabled = !g_log_enabled;  /* toggle logging */
      }
      vTaskDelay(pdMS_TO_TICKS(300));
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static void StartSdcardTask(void *argument)
{
  char line[64];
  static int first_run = 1;
  char tempStr[16];

  vTaskDelay(pdMS_TO_TICKS(500)); /* system settle */

  if (first_run)
  {
    first_run = 0;

    Mount_SD("/");
    Create_File("log.txt");
    Update_File("log.txt", "SYSTEM BOOT\n");
    Unmount_SD("/");
  }

  for (;;)
  {
    if (state == Working_Mode && g_log_enabled)
    {
      format_temperature(tempStr, sizeof(tempStr), g_temperature);

      uint32_t t_ms  = xTaskGetTickCount() * portTICK_PERIOD_MS;
      uint32_t t_sec = t_ms / 1000U;

      snprintf(line, sizeof(line), "%lu: %s degC\r\n",
               (unsigned long)t_sec,
               tempStr);

      Mount_SD("/");
      Update_File("log.txt", line);
      Unmount_SD("/");

      vTaskDelay(pdMS_TO_TICKS(10000)); /* log every 10 sec */
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(200)); /* idle */
    }
  }
}
