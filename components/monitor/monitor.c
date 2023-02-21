/*
 * monitor.c
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "monitor.h"

TaskHandle_t monitorTaskHandle = NULL;

static void monitor_handler(void *arg)
{
	static char buf[1024];
	for (;;)
	{
		vTaskGetRunTimeStats(buf);
		ESP_LOGI("MONITOR","Run Time Stats:\nTask Name    Time    Percent\n%s\n", buf);
		vTaskDelay(2000/portTICK_RATE_MS);
	}
}

void monitor_task_start_up(void)
{
	xTaskCreate(monitor_handler, "monitor", 3072, NULL, 0, &monitorTaskHandle);
    return;
}


void monitor_task_stop(void)
{
    vTaskDelete(monitorTaskHandle);
    return;
}
