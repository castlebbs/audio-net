/*
 * file_reader.c
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

 /*
  * File reader: Read a file and output its content into a ringbuffer
  *
  * Behavior controlled by message in queue
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

#include "file_reader.h"
#include "audio_dispatch.h"

static const char* TAG = "FILE_READER";
static const char* FILENAME = "/sdcard/looper.wav";

static xTaskHandle s_file_reader_task_handle = NULL;
static FILE* file;
static char current_filename[32]; // TODO optimize this

#define ARING_BUF_LEN (1024*16)
#define ARING_BUF_COUNT 2

static aring_buffer_t* aring_init(int aring_buf_count, int aring_buf_len)
{
	aring_buffer_t* aring = (aring_buffer_t*)malloc(sizeof(aring_buffer_t));
	memset(aring, 0, sizeof(aring_buffer_t));

	aring->queue = xQueueCreate(10, sizeof(int));

	int bux_idx;
	aring->buf = (char**)malloc(sizeof(char*) * aring_buf_count);
	for (bux_idx = 0; bux_idx < ARING_BUF_COUNT; bux_idx++) {
		//TODO See if allocating in DMA space would reduce CPU, need to do tests
		//     because sdmmc driver uses DMA
		aring->buf[bux_idx] = (char*)calloc(1, aring_buf_len);
		if (aring->buf[bux_idx] == NULL) {
			ESP_LOGE("ARING", "Error malloc aring buffer");
			return NULL;
		}
		ESP_LOGI("ARING", "Addr[%d] = %d", bux_idx, (int)aring->buf[bux_idx]);
	}
	aring->rw_pos = 0;
	aring->buf_size = aring_buf_len;
	aring->buf_count = aring_buf_count;
	aring->current_buf = 0;
	return aring;
}

static void open_assign_file(char* filename)
{
	if (xSemaphoreTake(file_mutex, (portTickType)portMAX_DELAY)) {
		file = fopen(FILENAME, "r");
		if (!file) ESP_LOGE(TAG, "Can't open file");
		strcpy(current_filename, FILENAME);
	}
}

static void close_file()
{
	struct stat st;
	stat(current_filename, &st);
	fsync(fileno(file));
	fclose(file);
	xSemaphoreGive(file_mutex);
	ESP_LOGI(TAG, "Closing file [name=%s] [size=%d]", current_filename, (int)st.st_size);
}

static void prefill_buffers()
{
	for (int i = 0; i < file_reader_aring->buf_count; i++)
	{
		fread(file_reader_aring->buf[i], 1, ARING_BUF_LEN, file);
	}
}

static void file_reader_task_handler(void* arg)
{
	ESP_LOGI(TAG, "Starting file_reader task");
	ESP_LOGI(TAG, "Initializing aring structure");
	file_reader_aring = aring_init(ARING_BUF_COUNT, ARING_BUF_LEN);

	// Aring buffer number
	int buffer_number = 0;

	size_t read_len; // Number of bytes returned to sd_buf by fread
	file_reader_msg_t msg;
	TickType_t wait = (portTickType)portMAX_DELAY;

	for (;;) {
		if (xQueueReceive(file_reader_cmd_queue, &msg, wait) == pdTRUE) {
			if ((&msg)->action == START && file_reader_status == STOPPED) {
				// Open file
				ESP_LOGI(TAG, "Asign file [filename=]");
				open_assign_file((&msg)->filename);

				// Send messages to pre-fill two buffers
				ESP_LOGI(TAG, "Pre-fill buffers");
				prefill_buffers();

				ESP_LOGI(TAG, "Change status to RUNNING");
				file_reader_status = RUNNING; //TODO investigate the use of mutex here
				wait = 0;
			}
			if ((&msg)->action == STOP && file_reader_status == RUNNING) {
				ESP_LOGI(TAG, "Closing file");
				close_file();
				ESP_LOGI(TAG, "Change status to STOPPED");
				file_reader_status = STOPPED;
				wait = (portTickType)portMAX_DELAY;
				continue; // Return beginning of the loop
			}
		}

		// The below is always run when file_reader_status is set to RUNNING

		// Need to have a buffer available to write into it. This is sent by the task reading
		//from the buffer
		if (xQueueReceive(file_reader_aring->queue, &buffer_number, portMAX_DELAY) == pdPASS)
		{
			read_len = fread(file_reader_aring->buf[buffer_number], 1, ARING_BUF_LEN, file);
			if (read_len <= 0) // We loop
			{
				fseek(file, 0, SEEK_SET);
				continue;
			}
		}

	}
}


void file_reader_task_shut_down(void)
{
	ESP_LOGI(TAG, "Stopping file_reader task");
	if (s_file_reader_task_handle) {
		vTaskDelete(s_file_reader_task_handle);
		s_file_reader_task_handle = NULL;
	}

	if (s_ringbuf_i2s_file_playback) {
		vRingbufferDelete(s_ringbuf_i2s_file_playback);
		s_ringbuf_i2s_file_playback = NULL;
	}

}

void file_reader_task_start_up(void)
{
	xTaskCreate(file_reader_task_handler, "file_reader", 1024 * 4, NULL, 22, &s_file_reader_task_handle);
	return;
}
