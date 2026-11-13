/*******************************************************************************
* File Name : app_main.c
*
* Layer 3 - execution (FreeRTOS). No HAL/LL calls or pins here (board/).
*
* hello-world: helloTask prints "hello, world" every 1 s (vTaskDelayUntil: no
* drift); the console task answers info / tasks. Same console contract as the
* bare-metal skill's hello-world, so the same test spec passes under both.
*******************************************************************************/
#include <stdio.h>

#include "app_main.h"
#include "app_rtos.h"
#include "board.h"
#include "console.h"

#define APP_NAME            "hello-world"
#define APP_VERSION         "1.0"
#define HELLO_PERIOD_MS     (1000U)
#define PRIO_HELLO          (tskIDLE_PRIORITY + 2U)

static void print_info(void)
{
    printf("INFO app=%s v=%s board=%s period_ms=%u rtos=FreeRTOS %s\r\n", APP_NAME, APP_VERSION,
           BOARD_NAME, (unsigned)HELLO_PERIOD_MS, tskKERNEL_VERSION_NUMBER);
    printf("READY\r\n");
}

static void hello_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(HELLO_PERIOD_MS));
        printf("hello, world\r\n");
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

    printf("\r\n[APP] %s %s on %s (FreeRTOS)\r\n", APP_NAME, APP_VERSION, BOARD_NAME);
    print_info();

    (void)app_rtos_task(hello_task, "helloTask", APP_RTOS_STACK_PRINTF, PRIO_HELLO, NULL);
    app_rtos_console_start(commands, sizeof(commands) / sizeof(commands[0]));
    app_rtos_run();
}
