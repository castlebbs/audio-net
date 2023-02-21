/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifndef _AUDIO_BOARD_DEFINITION_H_
#define _AUDIO_BOARD_DEFINITION_H_

//DR

/* SD card related */
#define SD_CARD_INTR_GPIO           GPIO_NUM_34
#define SD_CARD_INTR_SEL            GPIO_SEL_34
#define SD_CARD_OPEN_FILE_NUM_MAX   5
 
//#define GPIO_AUXIN_DETECT           39
 
/* LED indicators */
#define GPIO_LED_GREEN              22
#define GPIO_LED_RED                -1
 
/* I2C gpios */
#define IIC_CLK                     32
#define IIC_DATA                    33


/* PA */
#define GPIO_PA_EN                  GPIO_NUM_21
#define GPIO_SEL_PA_EN              GPIO_SEL_21
 
/* Press button related */
#define GPIO_SEL_REC                GPIO_SEL_36    //SENSOR_VP
#define GPIO_SEL_MODE               GPIO_SEL_13    //SENSOR_VN
#define GPIO_REC                    GPIO_NUM_36
#define GPIO_MODE                   GPIO_NUM_13
 
/* Touch pad related */
#define TOUCH_SEL_SET               GPIO_SEL_19
#define TOUCH_SEL_PLAY              GPIO_SEL_23
#define TOUCH_SEL_VOLUP             GPIO_SEL_18
#define TOUCH_SEL_VOLDWN            GPIO_SEL_5

#define TOUCH_SET                   GPIO_NUM_19
#define TOUCH_PLAY                  GPIO_NUM_23
#define TOUCH_VOLUP                 GPIO_NUM_18
#define TOUCH_VOLDWN                GPIO_NUM_5
 
/* I2S gpios
#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25
#define IIS_DOUT                    35
*/

//DR


#define BUTTON_2_ID               GPIO_NUM_13
#define BUTTON_3_ID               GPIO_NUM_19
#define BUTTON_4_ID               GPIO_NUM_23
#define BUTTON_5_ID               GPIO_NUM_18
#define BUTTON_6_ID               GPIO_NUM_5


#define PA_ENABLE_GPIO            GPIO_NUM_21 //GPIO_NUM_12
#define ADC_DETECT_GPIO           GPIO_NUM_36
#define BATTERY_DETECT_GPIO       GPIO_NUM_37

#define SDCARD_OPEN_FILE_NUM_MAX  5
#define SDCARD_INTR_GPIO          GPIO_NUM_34


extern audio_hal_func_t AUDIO_NEW_CODEC_DEFAULT_HANDLE;

#define AUDIO_CODEC_DEFAULT_CONFIG(){                   \
        .adc_input  = AUDIO_HAL_ADC_INPUT_LINE1,        \
        .dac_output = AUDIO_HAL_DAC_OUTPUT_ALL,         \
        .codec_mode = AUDIO_HAL_CODEC_MODE_BOTH,        \
        .i2s_iface = {                                  \
            .mode = AUDIO_HAL_MODE_SLAVE,               \
            .fmt = AUDIO_HAL_I2S_NORMAL,                \
            .samples = AUDIO_HAL_44K_SAMPLES,           \
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,        \
        },                                              \
};

#define INPUT_KEY_NUM     5

#define INPUT_KEY_DEFAULT_INFO() {                      \
    {                                                   \
        .type = PERIPH_ID_BUTTON,                       \
        .user_id = INPUT_KEY_USER_ID_2,                 \
        .act_id = BUTTON_2_ID,                          \
    },                                                  \
    {                                                   \
        .type = PERIPH_ID_BUTTON,                       \
        .user_id = INPUT_KEY_USER_ID_3,                 \
        .act_id = BUTTON_3_ID,                          \
    },                                                  \
    {                                                   \
        .type = PERIPH_ID_BUTTON,                       \
        .user_id = INPUT_KEY_USER_ID_4,                 \
        .act_id = BUTTON_4_ID,                          \
    },                                                  \
    {                                                   \
        .type = PERIPH_ID_BUTTON,                       \
        .user_id = INPUT_KEY_USER_ID_5,                 \
        .act_id = BUTTON_5_ID,                          \
    },                                                  \
    {                                                   \
        .type = PERIPH_ID_BUTTON,                       \
        .user_id = INPUT_KEY_USER_ID_6,                 \
        .act_id = BUTTON_6_ID,                          \
    },                                                  \
}

#endif
