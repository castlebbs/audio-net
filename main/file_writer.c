/*
 * file_writer.c
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */


 /*
  * Action that the file writer task needs to support:
  *
  *  - Start a recording:
  *  	Open file with specified ID
  *  	Read from ring buffer and writer to file
  *  	Close the file
  */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"
#include "sdcard.h"

#include "file_writer.h"
#include "file_reader.h"

static const char* TAG = "FILE_WRITER";
static const char* PREFIX = "/sdcard/loop";

static const char* FILENAME = "/sdcard/looper.wav";


static xTaskHandle s_file_writer_task_handle = NULL;
static FILE* file;
static char current_filename[32]; // TODO optimize this


static void open_assign_file()
{
	if (xSemaphoreTake(file_mutex, (portTickType)portMAX_DELAY)) {
		file = fopen(FILENAME, "w");
		if (!file) ESP_LOGE(TAG, "Can't open file");
	}
	strcpy(current_filename, FILENAME);

}

static void close_file()
{
	// Write the wav header at the beginning of the file
	struct stat st;
	stat(current_filename, &st);
	fsync(fileno(file));
	fclose(file);
	xSemaphoreGive(file_mutex);
	ESP_LOGI(TAG, "Saved file [name=%s] [size=%d]", current_filename, (int)st.st_size);
}

static void file_writer_task_handler(void* arg)
{
	ESP_LOGI(TAG, "Starting file_writer task");
	uint8_t* data = NULL;
	size_t item_size = 0;
	int sd_buf_len = 1024 * 8;
	int received_len;
	char* sd_buf;
	sd_buf = malloc(sd_buf_len);

	file_writer_msg_t msg;
	file_writer_status = STOPPED;
	TickType_t wait = (portTickType)portMAX_DELAY;

	for (;;) {
		if (xQueueReceive(file_writer_cmd_queue, &msg, wait) == pdTRUE) {
			if ((&msg)->action == START && file_writer_status == STOPPED) {
				open_assign_file();
				file_writer_status = RUNNING;
				wait = 0;
			}
			else if ((&msg)->action == STOP && file_writer_status == RUNNING) {
				close_file();
				file_writer_status = STOPPED;
				wait = (portTickType)portMAX_DELAY;
				continue; // Return beginning of the loop
			}
			else { // We ignore this message
				continue;
			}
		}

		received_len = 0;
		while (received_len < sd_buf_len) {
			data = (uint8_t*)xRingbufferReceiveUpTo(s_ringbuf_file_writer, &item_size, portMAX_DELAY, sd_buf_len - received_len);
			if (item_size > 0) // We received data
			{
				memcpy(sd_buf + received_len, data, item_size);
				received_len += item_size;
				vRingbufferReturnItem(s_ringbuf_file_writer, (void*)data);
			}
		}
		if (received_len > 0)
		{
			fwrite(sd_buf, 1, received_len, file);
			fsync(fileno(file));
		}
	}
}

void file_writer_task_shut_down(void)
{
	ESP_LOGI(TAG, "Stopping file_writer task");
	if (s_file_writer_task_handle) {
		vTaskDelete(s_file_writer_task_handle);
		s_file_writer_task_handle = NULL;
	}

	if (s_ringbuf_file_writer) {
		vRingbufferDelete(s_ringbuf_file_writer);
		s_ringbuf_file_writer = NULL;
	}
}

void file_writer_task_start_up(void)
{
	xTaskCreate(file_writer_task_handler, "file_writer", 1024 * 5, NULL, 19, &s_file_writer_task_handle);
	return;
}
