/*
 * i2s_writer.h
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#ifndef MAIN_I2S_WRITER_H_
#define MAIN_I2S_WRITER_H_
#include "audio_types.h"

RingbufHandle_t s_ringbuf_i2s_writer;


//xQueueHandle file_writer_cmd_queue = NULL;
void i2s_writer_task_start_up(void);

#endif /* MAIN_I2S_WRITER_H_ */

