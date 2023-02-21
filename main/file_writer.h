/*
 * file_writer.h
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#ifndef MAIN_FILE_WRITER_H_
#define MAIN_FILE_WRITER_H_

#include "freertos/ringbuf.h"
#include "audio_dispatch.h"
#include "audio_types.h"

typedef struct {
	audio_action_t action;
	char filename[30];
} file_writer_msg_t;

audio_status_t file_writer_status;
RingbufHandle_t s_ringbuf_file_writer;
xQueueHandle file_writer_cmd_queue;

void file_writer_task_start_up(void);
void file_writer_task_shut_down(void);

#endif /* MAIN_FILE_WRITER_H_ */
