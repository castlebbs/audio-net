/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "console.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "sys/queue.h"
#include "argtable3/argtable3.h"
#include "sdcard.h"
#include "monitor.h"
#include "audio_mem.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#endif

static const char *TAG = "CONSOLE";

#define CONSOLE_MAX_ARGUMENTS (5)

static const int STOPPED_BIT = BIT1;



static char *conslole_parse_arguments(char *str, char **saveptr)
{
    char *p;

    if (str != NULL) {
        *saveptr = str;
    }

    p = *saveptr;
    if (!p) {
        return NULL;
    }
    /* Skipping white space.*/
    while (*p == ' ' || *p == '\t') {
        p++;
    }

    if (*p == '"') {
        /* If an argument starts with a double quote then its delimiter is another quote.*/
        p++;
        *saveptr = strstr(p, "\"");
    } else {
        /* The delimiter is white space.*/
        *saveptr = strpbrk(p, " \t");
    }

    /* Replacing the delimiter with a zero.*/
    if (*saveptr != NULL) {
        *(*saveptr)++ = '\0';
    }
    return *p != '\0' ? p : NULL;
}

bool console_get_line(periph_console_handle_t console, unsigned max_size, TickType_t time_to_wait)
{
    char c;
    char tx[3];

#if defined(ESP_IDF_VERSION)
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
    int nread = uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, (uint8_t *)&c, 1, time_to_wait);  
#else
#endif
#else
    int nread = uart_read_bytes(CONFIG_CONSOLE_UART_NUM, (uint8_t *)&c, 1, time_to_wait);
#endif
    if (nread <= 0) {
        return false;
    }
    if ((c == 8) || (c == 127)) {  // backspace or del
        if (console->total_bytes > 0) {
            console->total_bytes --;
            tx[0] = c;
            tx[1] = 0x20;
            tx[2] = c;

#if defined(ESP_IDF_VERSION)
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
            uart_write_bytes(CONFIG_ESP_CONSOLE_UART_NUM, (const char *)tx, 3);
#else
#endif
#else
            uart_write_bytes(CONFIG_CONSOLE_UART_NUM, (const char *)tx, 3);
#endif

        }
        return false;
    }
    if (c == '\n' || c == '\r') {
        tx[0] = '\r';
        tx[1] = '\n';

#if defined(ESP_IDF_VERSION)
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
        uart_write_bytes(CONFIG_ESP_CONSOLE_UART_NUM, (const char *)tx, 2);
#else
#endif
#else
        uart_write_bytes(CONFIG_CONSOLE_UART_NUM, (const char *)tx, 2);
#endif
        console->buffer[console->total_bytes] = 0;
        return true;
    }

    if (c < 0x20) {
        return false;
    }

#if defined(ESP_IDF_VERSION)
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
    uart_write_bytes(CONFIG_ESP_CONSOLE_UART_NUM, (const char *)&c, 1);
#else
#endif
#else
    uart_write_bytes(CONFIG_CONSOLE_UART_NUM, (const char *)&c, 1);
#endif

    console->buffer[console->total_bytes++] = (char)c;
    if (console->total_bytes > max_size) {
        console->total_bytes = 0;
    }

    return false;
}

static bool console_exec(periph_console_handle_t console, char *cmd, int argc, char *argv[])
{
    int i;
    for (i = 0; i < console->command_num; i++) {
        if (strcasecmp(cmd, console->commands[i].cmd) == 0) {
            if (console->commands[i].func) {
                console->commands[i].func(console, argc, argv);
                return true;
            }
            int cmd_id = console->commands[i].id;
            if (cmd_id == 0) {
                cmd_id = i;
            }
            //esp_periph_send_event(self, cmd_id, argv, argc);
            return true;
        }
    }
    printf("----------------------\r\n");
    printf("Perpheral console HELP\r\n");
    printf("----------------------\r\n");
    for (i = 0; i < console->command_num; i++) {
        printf("%s \t%s\r\n", console->commands[i].cmd, console->commands[i].help);
    }
    return false;
}

esp_err_t console_destroy()
{
    console->run = false;
    xEventGroupWaitBits(console->state_event_bits, STOPPED_BIT, false, true, portMAX_DELAY);
    vEventGroupDelete(console->state_event_bits);
    if (console->prompt_string) {
        free(console->prompt_string);
    }
    free(console->buffer);
    free(console);
    return ESP_OK;
}

static void _console_task(void *pv)
{
	periph_console_handle_t console = (periph_console_handle_t)pv;
    char *lp, *cmd, *tokp;
    char *args[CONSOLE_MAX_ARGUMENTS + 1];
    int n;

    if (console->total_bytes >= console->buffer_size) {
        console->total_bytes = 0;
    }
    console->run = true;
    xEventGroupClearBits(console->state_event_bits, STOPPED_BIT);
    const char *prompt_string = CONSOLE_DEFAULT_PROMPT_STRING;
    if (console->prompt_string) {
        prompt_string = console->prompt_string;
    }
    printf("\r\n%s ", prompt_string);
    while (console->run) {
        if (console_get_line(console, console->buffer_size, 10 / portTICK_RATE_MS)) {
            if (console->total_bytes) {
                ESP_LOGD(TAG, "Read line: %s", console->buffer);
            }
            lp = conslole_parse_arguments(console->buffer, &tokp);
            cmd = lp;
            n = 0;
            while ((lp = conslole_parse_arguments(NULL, &tokp)) != NULL) {
                if (n >= CONSOLE_MAX_ARGUMENTS) {
                    printf("too many arguments\r\n");
                    cmd = NULL;
                    break;
                }
                args[n++] = lp;
            }
            args[n] = NULL;
            if (console->total_bytes > 0) {
                console_exec(console, cmd, n, args);
                console->total_bytes = 0;
            }
            printf("%s ", prompt_string);
        }

    }
    xEventGroupSetBits(console->state_event_bits, STOPPED_BIT);
    vTaskDelete(NULL);
}
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
static esp_err_t run_time_stats(periph_console_handle_t periph, int argc, char *argv[])
{
    static char buf[1024];
    vTaskGetRunTimeStats(buf);
    printf("Run Time Stats:\nTask Name    Time    Percent\n%s\n", buf);
    return ESP_OK;
}
#endif

#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
static esp_err_t task_list(periph_console_handle_t periph, int argc, char *argv[])
{
    static char buf[1024];
    vTaskList(buf);
    printf("Task List:\nTask Name    Status   Prio    HWM    Task Number\n%s\n", buf);
    return ESP_OK;
}
#endif


static esp_err_t show_free_mem(periph_console_handle_t periph, int argc, char *argv[])
{
    AUDIO_MEM_SHOW(TAG);
    return ESP_OK;
}

static esp_err_t cli_mon_on(periph_console_handle_t console, int argc, char *argv[])
{
    ESP_LOGI("COMMAND", "Starting monitor");
    monitor_task_start_up();
    return ESP_OK;
}

static esp_err_t cli_mon_off(periph_console_handle_t console, int argc, char *argv[])
{
    ESP_LOGI("COMMAND", "Stopping monitor");
    monitor_task_stop();
    return ESP_OK;
}

static esp_err_t cli_sdcard_mount(periph_console_handle_t console, int argc, char *argv[])
{
    ESP_LOGI("COMMAND", "Mounting SD");
    sdcard_mount();
    return ESP_OK;
}

static esp_err_t cli_sdcard_umount(periph_console_handle_t console, int argc, char *argv[])
{
    ESP_LOGI("COMMAND", "Umounting SD");
    sdcard_umount();
    return ESP_OK;
}

static esp_err_t cli_list_files(periph_console_handle_t console, int argc, char *argv[])
{
    ESP_LOGI("COMMAND", "List files");
    sdcard_list_files();
    return ESP_OK;
}
const periph_console_cmd_t cli_cmd[] = {
    { .cmd = "mon",       .id = 1, .help = "Start monitor",                    .func = cli_mon_on },
    { .cmd = "moff",      .id = 2, .help = "Stop monitor",                   .func = cli_mon_off },
    { .cmd = "m",      .id = 3, .help = "Mount sd card",                   .func = cli_sdcard_mount },
    { .cmd = "u",      .id = 4, .help = "Unmount sd card",                   .func = cli_sdcard_umount },
    { .cmd = "l",      .id = 5, .help = "List files on sd",                   .func = cli_list_files },
    { .cmd = "free",        .id = 6, .help = "Get system free memory",                         .func = show_free_mem },
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    { .cmd = "stat",        .id = 7, .help = "Show processor time of all FreeRTOS tasks",      .func = run_time_stats },
#endif
#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
    { .cmd = "tasks",       .id = 8, .help = "Get information about running tasks",            .func = task_list },
#endif

};

esp_err_t console_init()
{
	periph_console_cfg_t config = {
			.command_num = sizeof(cli_cmd) / sizeof(periph_console_cmd_t),
			.commands = cli_cmd,
			.buffer_size = 384,
	};

	console = calloc(1, sizeof(periph_console_t));
	AUDIO_MEM_CHECK(TAG, console, return ESP_FAIL);
	console->commands = (&config)->commands;
	console->command_num = (&config)->command_num;
    console->task_stack = CONSOLE_DEFAULT_TASK_STACK;
    console->task_prio = CONSOLE_DEFAULT_TASK_PRIO;
    console->buffer_size = CONSOLE_DEFAULT_BUFFER_SIZE;
     if ((&config)->buffer_size > 0) {
        console->buffer_size = (&config)->buffer_size;
    }
    if ((&config)->task_stack > 0) {
        console->task_stack = (&config)->task_stack;
    }
    if ((&config)->task_prio) {
        console->task_prio = (&config)->task_prio;
    }
    if ((&config)->prompt_string) {
        console->prompt_string = strdup((&config)->prompt_string);
        AUDIO_MEM_CHECK(TAG, console->prompt_string, {
            free(console);
            return ESP_FAIL;
        });
    }
    console->state_event_bits = xEventGroupCreate();

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

#if defined(ESP_IDF_VERSION)
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
    uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, console->buffer_size * 2, 0, 0, NULL, 0);
#else
#endif
#else
    uart_driver_install(CONFIG_CONSOLE_UART_NUM, console->buffer_size * 2, 0, 0, NULL, 0);
#endif

    /* Tell VFS to use UART driver */
#if defined(ESP_IDF_VERSION)
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0))
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
#else
#endif
#else
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
#endif

    console->buffer = (char *) malloc(console->buffer_size);
    AUDIO_MEM_CHECK(TAG, console->buffer, {
        return ESP_ERR_NO_MEM;
    });

    if (xTaskCreate(_console_task, "console_task", console->task_stack, console, console->task_prio, NULL) != pdTRUE) {
        ESP_LOGE(TAG, "Error create console task, memory exhausted?");
        return ESP_FAIL;
    }
    return ESP_OK;
}
