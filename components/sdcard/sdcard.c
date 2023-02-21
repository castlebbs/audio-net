/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#include "driver/sdmmc_host.h"
#endif

static const char *TAG = "SDCARD";

#define MOUNT_POINT "/sdcard"
#define SDCARD_FILE_PREV_NAME           "file:/"
#define SDCARD_SCAN_URL_MAX_LENGTH      (1024 * 2)
//#define INTERNAL_FLASH 1

void sdcard_mount(void)
{

#ifndef INTERNAL_FLASH
    esp_err_t ret;
    sdmmc_card_t* card;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
		.force_format = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    //host.flags = SDMMC_HOST_FLAG_4BIT;
    //host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    //slot_config.gpio_cd = g_gpio;
    slot_config.width = 1;

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

//    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
//    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
//    // does make a difference some boards, so we do that here.
//    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
//    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
//    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
//    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
//    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);


    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    // All done, unmount partition and disable SDMMC or SPI peripheral
    //
    //ESP_LOGI(TAG, "Card unmounted");

#else
	#include "esp_spiffs.h"
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/sdcard",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
#endif

}

void sdcard_umount(void)
{
	esp_vfs_fat_sdmmc_unmount();
}

void sdcard_list_files()
{
    char *file_url = calloc(1, SDCARD_SCAN_URL_MAX_LENGTH);

    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Open [%s] directory failed", MOUNT_POINT);
        free(file_url);
        return;
    }

    struct dirent *file_info = NULL;
    while (NULL != (file_info = readdir(dir))) {
        if ((strlen(file_info->d_name) + strlen(MOUNT_POINT)) > (SDCARD_SCAN_URL_MAX_LENGTH - strlen(SDCARD_FILE_PREV_NAME))) {
            ESP_LOGE(TAG, "The file name is too long, invalid url");
            continue;
        }
        if (file_info->d_name[0] == '.') {
            continue;
        }

        if (file_info->d_type == DT_DIR) {
            if (file_info->d_name[0] == '_' && file_info->d_name[1] == '_') {
                continue;
            }
            memset(file_url, 0, SDCARD_SCAN_URL_MAX_LENGTH);
            printf("%s/%s\n", MOUNT_POINT, file_info->d_name);
        } else {
            memset(file_url, 0, SDCARD_SCAN_URL_MAX_LENGTH);
            printf("%s%s/%s\n", SDCARD_FILE_PREV_NAME, MOUNT_POINT, file_info->d_name);

            char *detect = strrchr(file_info->d_name, '.');
            if (NULL == detect) {
                continue;
            }
            detect ++;
            }
        }
    free(file_url);
    closedir(dir);
}
