/*
 * audio_dispatch.h
 *
 *      Author: David Robert
 *		License: Apache-2.0
 */

#ifndef MAIN_AUDIO_DISPATCH_H_
#define MAIN_AUDIO_DISPATCH_H_

#include "freertos/semphr.h"

void audio_dispatch_init();

void bt_write_ringbuf(const uint8_t* data, size_t size);

SemaphoreHandle_t file_mutex;

#endif /* MAIN_AUDIO_DISPATCH_H_ */
