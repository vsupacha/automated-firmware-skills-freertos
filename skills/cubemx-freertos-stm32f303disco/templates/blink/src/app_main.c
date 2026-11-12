/*******************************************************************************
* File Name : app_main.c
*
* Layer 3 - execution (FreeRTOS). No HAL/LL calls or pins here (board/).
*
* blink: ledTask toggles LED1 and prints "BLINK led1=x" every 0.5 s
* (vTaskDelayUntil: no drift); the console task answers info / tasks. Same
* console contract as the bare-metal skill's blink.
*******************************************************************************/
#include <stdio.h>

#include "app_main.h"
#include "app_rtos.h"
#include "board.h"
#include "console.h"
#include "led.h"

#define APP_NAME            "blink"
#define APP_VERSION         "1.0"
#define BLINK_PERIOD_MS     (500U)
#define PRIO_LED            (tskIDLE_PRIORITY + 2U)

static void print_info(void)
{
    printf("INFO app=%s v=%s board=%s period_ms=%u rtos=FreeRTOS %s\r\n", APP_NAME, APP_VERSION,
           BOARD_NAME, (unsigned)BLINK_PERIOD_MS, tskKERNEL_VERSION_NUMBER);
    printf("READY\r\n");
}

static void led_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(BLINK_PERIOD_MS));
        (void)led_toggle(1);
        printf("BLINK led1=%d\r\n", led_get(1) ? 1 : 0);
    }
}

static void cmd_info(int argc, char *argv[]) { (void)argc; (void)argv; print_info(); }

static const console_cmd_t commands[] =
{
    { "info", "info                  - app/board info", cmd_info },
    APP_RTOS_CMD_TASKS,
};

void app_main(void)
{
    board_init();
    led_init();

    printf("\r\n[APP] %s %s on %s (FreeRTOS)\r\n", APP_NAME, APP_VERSION, BOARD_NAME);
    print_info();

    (void)app_rtos_task(led_task, "ledTask", APP_RTOS_STACK_PRINTF, PRIO_LED, NULL);
    app_rtos_console_start(commands, sizeof(commands) / sizeof(commands[0]));
    app_rtos_run();
}
