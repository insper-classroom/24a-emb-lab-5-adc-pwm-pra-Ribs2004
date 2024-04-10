/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

int dead_zone(int val) {
    if (val > -150 && val < 150) {
        return 0;
    }
    return val;
}

void x_task(void *p) {
    adc_t data;
    adc_init();
    adc_gpio_init(27);

    while (1) {
        data.axis = 0;
        adc_select_input(1);
        data.val = dead_zone((adc_read() - 2048) / 8) / 10;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void y_task(void *p) {
    adc_t data;
    adc_init();
    adc_gpio_init(26);

    while (1) {
        data.axis = 1;
        adc_select_input(0);
        data.val = dead_zone((adc_read() - 2048) / 8) / 10;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);
        // printf("\naxis: %d, %d\n", data.axis, data.val);
        write_package(data);
        // printf("Axis: %d, Value: %d\n", data.axis, data.val);
    }
}


int main() {
    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}