/*
 * i2s_reader.h
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#ifndef MAIN_I2S_READER_H_
#define MAIN_I2S_READER_H_
#include "audio_types.h"

RingbufHandle_t s_ringbuf_i2s_reader_output;


//xQueueHandle file_writer_cmd_queue = NULL;
void i2s_reader_task_start_up(void);

#endif /* MAIN_I2S_WRITER_H_ */

