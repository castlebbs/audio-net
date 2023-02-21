/*
 * audio_dispatch.c
 *
 *      Author: David Robert
 *		License: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"

#include "audio_dispatch.h"
#include "file_writer.h"
#include "i2s_writer.h"
#include "i2s_reader.h"
#include "file_reader.h"

static const char* TAG = "AUDIO_DISPATCH";

// Init buffers and and queues
void audio_dispatch_init()
{
	// Init Semaphores
	file_mutex = xSemaphoreCreateMutex();

	// Init file writer
	s_ringbuf_file_writer = xRingbufferCreate(16 * 1024, RINGBUF_TYPE_BYTEBUF);
	if (s_ringbuf_file_writer == NULL) {
		ESP_LOGE(TAG, "Can't allocated memory for file writer buffer");
		return;
	}

	// Init queues
	file_writer_cmd_queue = xQueueCreate(10, sizeof(file_writer_msg_t));
	file_reader_cmd_queue = xQueueCreate(10, sizeof(file_reader_msg_t));

	// Stop readers and writers
	file_reader_status = STOPPED;
	file_writer_status = STOPPED;
}

void bt_write_ringbuf(const uint8_t* data, size_t size)
{
	xRingbufferSend(s_ringbuf_i2s_writer, (void*)data, size, portMAX_DELAY);

	if (file_writer_status == RUNNING)
	{
		BaseType_t done = xRingbufferSend(s_ringbuf_file_writer, (void*)data, size, 0);
		if (done == pdFALSE)
		{
			ESP_LOGE(TAG, "file writer ring buffer overflow");
		}
	}
}
