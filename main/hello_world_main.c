/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"

bool flash = 0;
bool signal_change = 0;
esp_timer_handle_t timer_handle;
uint64_t timer_last_time = 0;

void config_gpios()
{
    gpio_config_t LED = {
        .pin_bit_mask = 1ULL << GPIO_NUM_33,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&LED);

    gpio_config_t TEST = {
        .pin_bit_mask = 1ULL << GPIO_NUM_1,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&TEST);
}

void IRAM_ATTR gpio_isr_handler()
{
    uint64_t timer_time = esp_timer_get_time();
    if (timer_time - timer_last_time > 50 * 1000)
    {
        if (signal_change)
        {
            esp_timer_start_periodic(timer_handle, 1000 * 100);
        }
        else
        {
            if (esp_timer_is_active(timer_handle))
            {
                esp_timer_stop(timer_handle);
            };
            gpio_set_level(GPIO_NUM_33, 0);
        }
        signal_change = !signal_change;
    }
    timer_last_time = timer_time;
};

void config_interrupt_handler()
{
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

    gpio_isr_handler_add(GPIO_NUM_1, gpio_isr_handler, NULL);
}

void timer_function()
{
    gpio_set_level(GPIO_NUM_33, flash);
    flash = !flash;
}

void config_timer()
{
    esp_timer_create_args_t timer_args = {
        .callback = timer_function,
        .dispatch_method = ESP_TIMER_TASK,
    };
    esp_timer_create(&timer_args, &timer_handle);
}

void app_main(void)
{
    config_gpios();
    config_timer();
    config_interrupt_handler();
    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
