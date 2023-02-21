/*
 * i2s_reader.c
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"

#include "driver/i2s.h"

#include "esp_err.h"
#include "esp_log.h"

#include "i2s_reader.h"
#include "file_writer.h"
#include "audio_dispatch.h"

static const char* TAG = "I2S_READER";
static xTaskHandle s_i2s_reader_task_handle = NULL;

static void i2s_reader_task_handler(void* arg)
{
	ESP_LOGI(TAG, "Starting i2s_reader task");
	int buffer_len = 1024 * 8;
	size_t read_len;
	char* buffer = malloc(buffer_len);

	//    TickType_t wait = (portTickType)portMAX_DELAY;

	for (;;) {
		//    	if (xQueueReceive(i2s_reader_cmd_queue, &msg, wait) == pdTRUE) {
		//    		if ((&msg)->action == START && i2s_reader_status == STOPPED) {
		//    			i2s_reader_status = RUNNING;
		//    			open_assign_file((&msg)->filename);
		//    			wait = 0;
		//    		}
		//    		if ((&msg)->action == STOP && i2s_reader_status == RUNNING) {
		//    			i2s_reader_status = STOPPED;
		//    			close_file();
		//    			wait = (portTickType)portMAX_DELAY;
		//    			continue; // Return beginning of the loop
		//    		}
		//    	}
		i2s_read(0, buffer, buffer_len, &read_len, portMAX_DELAY);

		if (read_len != buffer_len)
		{
			ESP_LOGE(TAG, "I2S reader read [bytes:%d]", (int)read_len);
		}
		if (read_len > 0) //TODO Need to handle errors
		{
			BaseType_t done = xRingbufferSend(s_ringbuf_i2s_reader_output, (void*)buffer, read_len, pdMS_TO_TICKS(1000));
			if (done == pdFALSE)
			{
				ESP_LOGE(TAG, "I2S reader ring buffer overflow");
			}

			// TODO Define a property of readers to where they need to output their data (outputrb like adf)

			if (file_writer_status == RUNNING)
			{
				BaseType_t done = xRingbufferSend(s_ringbuf_file_writer, (void*)buffer, read_len, 0);
				if (done == pdFALSE)
				{
					ESP_LOGE(TAG, "file writer ring buffer overflow");
				}
			}
			//ESP_LOGI(TAG, "Writing some data to i2s [length:%d]", read_len);
		}
	}
}

void i2s_reader_task_shut_down(void)
{
	ESP_LOGI(TAG, "Stopping i2s_reader task");
	if (s_i2s_reader_task_handle) {
		vTaskDelete(s_i2s_reader_task_handle);
		s_i2s_reader_task_handle = NULL;
	}

	if (s_ringbuf_i2s_reader_output) {
		vRingbufferDelete(s_ringbuf_i2s_reader_output);
		s_ringbuf_i2s_reader_output = NULL;
	}

}

void i2s_reader_task_start_up(void)
{
	xTaskCreate(i2s_reader_task_handler, "i2s_reader", 1024 * 4, NULL, 23, &s_i2s_reader_task_handle);
	return;
}
