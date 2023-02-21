/*
 * i2s_writer.c
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

 /*
  * States:
  * Initially, no playback nor recording (this is because we don't know the length of a loop
  *
  * Click record for the first time:                                  Record(file_id=0), Play(None)
  * Click record again (start to loop with overdub):                  Record(file_id=1), Play(file_id=0)
  * 	 Until click to stop overdub, when end of loop:                  Record(file_id=2), Play(file_id=1)
  * Click record again (to stop overdub and save): Once loop end: 	 Stop recording codex in file, next record filename is file_id_tmp
  *
  */

  /*
   *  Messages:
   *  	Next record name
   *  	Next play name
   */

   /*
	* Unless when the looper is standby, there is generally at least:
	* - 1 i2s input
	* - 1 file playing
	* - 1 file recording
	*
	* The file recording records the mix of file playback + input (overdub)
	*
	* Filenames are sequential numbers
	* file1
	* file2
	* filex
	*
	* When recording number is x, playing number is x-1
	* First recording (1), there is no playing.
	*/

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"

#include "i2s_writer.h"
#include "i2s_reader.h"
#include "file_reader.h"
#include "file_writer.h"

static const char* TAG = "I2S_WRITER";

static xTaskHandle s_i2s_writer_task_handle = NULL;

#define BUF_SIZE 256 // 64 * 4 (64 samples)

int16_t round_float_int(float in) {
	if (in >= 0.5)
		return (int16_t)in + 1;
	else
		return (int16_t)in;
}

int16_t limit(int16_t val) //TODO need to ensure this is necessary
{
	if (val > 32767)
		val = 32767;
	if (val < -32768)
		val = -32768;

	return  val;
}

void add_samples(uint32_t* bufa, uint32_t* bufb)
{
	const float div = (1.0f / 32768.0f);
	const float mul = (32768.0f);
	const uint32_t bit_mask_left = 0xFFFF, bit_mask_right = 0xFFFF0000;
	float la, lb, ra, rb;
	int16_t il, ir;
	uint32_t nsample = 0; // new samples
	for (int i = 0; i < BUF_SIZE; i++)
	{
		la = div * ((int16_t)(bit_mask_left & bufa[i]));
		lb = div * ((int16_t)(bit_mask_left & bufb[i]));
		ra = div * ((int16_t)((bit_mask_right & bufa[i]) >> 16));
		rb = div * ((int16_t)((bit_mask_right & bufb[i]) >> 16));
		la = (la + lb); ra = (ra + rb);
		la *= mul; ra *= mul;
		il = limit(round_float_int(la)); ir = limit(round_float_int(ra));
		nsample = (ir << 16) & bit_mask_right;
		nsample |= il & bit_mask_left;
		bufa[i] = nsample;
	}
}

static void i2s_writer_task_handler(void* arg)
{
	uint32_t* i2sdata = NULL;
	uint32_t* filedata = NULL;
	i2sdata = malloc(BUF_SIZE);
	filedata = malloc(BUF_SIZE);
	size_t bytes_written = 0;

	for (;;) {

		// Read from codec
		i2s_read(0, i2sdata, BUF_SIZE, &bytes_written, portMAX_DELAY);

		// Read from track and merge
		if (file_reader_status == RUNNING)
		{
			// Read from file_reader_aring
			memcpy(filedata, file_reader_aring->buf[file_reader_aring->current_buf] + file_reader_aring->rw_pos, BUF_SIZE);
			file_reader_aring->rw_pos += BUF_SIZE;
			if (file_reader_aring->rw_pos >= file_reader_aring->buf_size) // Need to switch buffer
			{
				// Inform filereader to fill the buffer we just processed
				xQueueSend(file_reader_aring->queue, &(file_reader_aring->current_buf), 0);

				// Set new buffer for next run
				if (++(file_reader_aring->current_buf) == file_reader_aring->buf_count) { file_reader_aring->current_buf = 0; }
				file_reader_aring->rw_pos = 0;
			}
			add_samples(i2sdata, filedata);
		}

		// Write the stream to file
		if (file_writer_status == RUNNING) xRingbufferSend(s_ringbuf_file_writer, (void*)i2sdata, BUF_SIZE, 0);

		// Write to i2s for output
		i2s_write(0, i2sdata, BUF_SIZE, &bytes_written, portMAX_DELAY);
	}
}

void i2s_writer_task_start_up(void)
{
	ESP_LOGI(TAG, "Starting i2s_writer task");
	xTaskCreate(i2s_writer_task_handler, "i2s_writer", 1024 * 2, NULL, configMAX_PRIORITIES - 3, &s_i2s_writer_task_handle);
	return;
}

void i2s_writer_task_shut_down(void)
{
	if (s_i2s_writer_task_handle) {
		vTaskDelete(s_i2s_writer_task_handle);
		s_i2s_writer_task_handle = NULL;
	}
	if (s_ringbuf_i2s_writer) {
		vRingbufferDelete(s_ringbuf_i2s_writer);
		s_ringbuf_i2s_writer = NULL;
	}
}

