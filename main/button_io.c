/*
 * button_io.c
 *
 *      Author: David Robert
 *		License: Apache-2.0
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "button.h"
#include "esp_log.h"
#include "console.h"
#include "file_writer.h"
#include "file_reader.h"
#include "loop_state.h"

#include "../components/lib/console/console.h"

static const char* TAG = "BUTTON_GPIO";
static xQueueHandle button_internal_gpio_queue = NULL;
static esp_button_handle_t button = NULL;

static void button_send_event(int event_id, uint64_t mask)
{
	int gpio_num = 0;
	file_writer_msg_t msg;
	file_reader_msg_t rmsg;

	while (mask) {
		if (mask & 0x01) {

			//    		if (gpio_num == 18 && event_id == 1)
			//    		{
			//    			ESP_LOGI(TAG,"Starting console");
			//    			console_init();
			//    		}
			//    		if (gpio_num == 18 && event_id == 2)
			//    		{
			//    			ESP_LOGI(TAG,"Stoping console");
			//    			console_destroy();
			//    		}
			if (gpio_num == 18 && event_id == 1)
			{
				ESP_LOGI(TAG, "Record button pressed");
				msg.action = START;
				strcpy(msg.filename, "none");
				xQueueSend(file_writer_cmd_queue, &msg, 0);
			}
			if (gpio_num == 5 && event_id == 1)
			{
				ESP_LOGI(TAG, "Stop Saving audio");
				msg.action = STOP;
				strcpy(msg.filename, "none");
				xQueueSend(file_writer_cmd_queue, &msg, 0);
				ESP_LOGI(TAG, "Playing Back");
				rmsg.action = START;
				strcpy(rmsg.filename, "none");
				xQueueSend(file_reader_cmd_queue, &rmsg, 0);
			}
			if (gpio_num == 5 && event_id == 2)
			{
				ESP_LOGI(TAG, "Stop playback");
				rmsg.action = STOP;
				strcpy(rmsg.filename, "none");
				xQueueSend(file_reader_cmd_queue, &rmsg, 0);
			}
		}
		mask >>= 1;
		gpio_num++;
	}
}


static void button_read_handler(void* arg)
{
	button_result_t result;
	int msg = 0;
	for (;;)
	{
		if (xQueueReceive(button_internal_gpio_queue, &msg, portMAX_DELAY))
		{
			if (button_read(button, &result)) {
				//ESP_LOGI(TAG, "Button event, press_mask %llx, release_mask: %llx, long_press_mask: %llx, long_release_mask: %llx",
				//		result.press_mask, result.release_mask, result.long_press_mask, result.long_release_mask);
				//button_send_event(self, PERIPH_BUTTON_PRESSED, result.press_mask);
				button_send_event(1, result.release_mask);
				button_send_event(2, result.long_press_mask);
				//button_send_event(self, PERIPH_BUTTON_LONG_RELEASE, result.long_release_mask);
			}
		}
	}
}

static void button_read_task_start_up(void)
{
	xTaskCreate(button_read_handler, "button_read", 4096, NULL, 1, NULL);
	return;
}

static void IRAM_ATTR button_intr_handler(void* arg)
{
	int dummy = 1;
	xQueueSendFromISR(button_internal_gpio_queue, &dummy, NULL);
}

static void button_timer_handler(void* tmr)
{
	//    esp_periph_handle_t periph = (esp_periph_handle_t) pvTimerGetTimerID(tmr);
	//    esp_periph_send_cmd_from_isr(periph, 0, NULL, 0);
	int dummy = 1;
	xQueueSendFromISR(button_internal_gpio_queue, &dummy, NULL);
}

void button_io_init()
{
	// Initialize button io queue
	button_internal_gpio_queue = xQueueCreate(5, sizeof(int));

	// Interrupt Service Handler
	gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

	// Initialize buttons
	button_config_t btn_config = {
			.gpio_mask = GPIO_SEL_5 | GPIO_SEL_18, // Button 6 and 5
			.long_press_time_ms = 500,
			.button_intr_handler = button_intr_handler,
			.intr_context = NULL,
	};
	button = button_init(&btn_config);
	button_read_task_start_up();
	TimerHandle_t timer = xTimerCreate("periph_itmer", 50 / portTICK_RATE_MS, pdTRUE, NULL, button_timer_handler);
	if (xTimerStart(timer, 0) != pdTRUE) {
		ESP_LOGE(TAG, "Error to starting timer");
	}
}

