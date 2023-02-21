/*
 * audio_types.h
 *
 *      Author: David Robert
 *		License: Apache-2.0
 */

#ifndef MAIN_AUDIO_TYPES_H_
#define MAIN_AUDIO_TYPES_H_

typedef struct
{
	int pos;
	int len;
	void* buf;
} file_reader_buffer_t;


typedef enum {
	RUNNING,
	STOPPED,
} audio_status_t;

// File reader message structure
typedef enum {
	STOP,
	START,
} audio_action_t;

#endif /* MAIN_AUDIO_TYPES_H_ */
