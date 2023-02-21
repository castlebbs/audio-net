/*
 * file_reader.h
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#ifndef MAIN_FILE_READER_H_
#define MAIN_FILE_READER_H_

#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "audio_dispatch.h"
#include "audio_types.h"

typedef struct {
	audio_action_t action;
	char* filename; //TODO verify if just storing the pointer is sufficient given it goes in a xQueue
} file_reader_msg_t;

typedef struct {
	char** buf;
	int buf_size;
	int buf_count;
	int rw_pos; // position relative to buffer
	int current_buf;
	QueueHandle_t queue;
} aring_buffer_t;

// ring buffer used by file_reader and i2s_writer
aring_buffer_t* file_reader_aring;

file_reader_buffer_t file_reader_buffer[3];
audio_status_t file_reader_status;
RingbufHandle_t s_ringbuf_i2s_file_playback;
xQueueHandle file_reader_cmd_queue;

void file_reader_task_start_up(void);
void file_reader_task_shut_down(void);

#endif /* MAIN_FILE_reader_H_ */
