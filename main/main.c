/*
 * main.c
 *
 *      Author: David Robert
 *      License : Apache - 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "driver/i2s.h"

#include "audio_hal.h"
#include "audio_mem.h"
#include "board.h"
#include "button_io.h"
#include "console.h"

#include "audio_dispatch.h"
#include "file_writer.h"
#include "file_reader.h"
#include "i2s_writer.h"
#include "i2s_reader.h"
#include "sdcard.h"


void app_main(void)
{
	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	// Init Audio Codec
	audio_hal_handle_t audio_hal_handle = audio_board_codec_init();
	audio_hal_ctrl_codec(audio_hal_handle, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

	// Init Button IO
	button_io_init();

	// Init esp32 I2S
	i2s_config_t i2s_config = {
		.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
		.sample_rate = 44100,
		.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
		.communication_format = I2S_COMM_FORMAT_I2S,
		.dma_buf_count = 4,
		.dma_buf_len = 32,
		.use_apll = 1,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2 | ESP_INTR_FLAG_IRAM,
		.tx_desc_auto_clear = true,
	};

	i2s_driver_install(0, &i2s_config, 0, NULL);
	i2s_pin_config_t i2s_pin_cfg = { 0 };
	get_i2s_pins(0, &i2s_pin_cfg);
	i2s_set_pin(0, &i2s_pin_cfg);


	sdcard_mount();

	ESP_LOGI("MAIN", "audio_dispatch_init");
	audio_dispatch_init();

	ESP_LOGI("MAIN", "file_reader_task_start_up");
	file_reader_task_start_up();

	ESP_LOGI("MAIN", "i2s_writer_task_start_up");
	i2s_writer_task_start_up();

	ESP_LOGI("MAIN", "file_writer_task_start_up");
	file_writer_task_start_up();

	// My monitor task
	//monitor_task_start_up();

	// Setup console
	//cli_setup_console();
}
