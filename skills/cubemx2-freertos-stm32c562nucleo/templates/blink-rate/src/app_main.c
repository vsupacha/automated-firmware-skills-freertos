/*******************************************************************************
* File Name : app_main.c
*
* Layer 3 - execution (FreeRTOS). No HAL/LL calls or pins here (board/), no
* rules here (func/blink_rate.c) - only tasks and how they talk.
*
* blink-rate: two tasks and one event
*   buttonTask (prio 3) samples BTN1 every PRESS_SAMPLE_MS (100 ms); each press
*              sends a "toggle" event to ledTask (task notification) and prints
*              "EVT btn1=pressed"
*   ledTask    (prio 2) toggles LED1 every blink_rate_ms() and prints
*              "BLINK led1=0|1"; each event steps the interval
*              1000 -> 500 -> 200 -> 1000 ms and prints "PERIOD ms=<n>"
*   console    (prio 1) info, tasks, period?, step (= the same event as a press,
*              for automatic tests)
*******************************************************************************/
#include <stdio.h>

#include "app_main.h"
#include "app_rtos.h"
#include "blink_rate.h"
#include "board.h"
#include "console.h"
#include "led.h"

#define APP_NAME        "blink-rate"
#define APP_VERSION     "1.0"
#define PRIO_LED        (tskIDLE_PRIORITY + 2U)
#define PRIO_BUTTON     (tskIDLE_PRIORITY + 3U)

static TaskHandle_t led_task;

static void print_info(void)
{
    printf("INFO app=%s v=%s board=%s period_ms=%lu sample_ms=%u rtos=FreeRTOS %s\r\n", APP_NAME,
           APP_VERSION, BOARD_NAME, (unsigned long)blink_rate_ms(), (unsigned)PRESS_SAMPLE_MS,
           tskKERNEL_VERSION_NUMBER);
    printf("READY\r\n");
}

/* the "toggle" event: one notification per press (counted, none lost) */
static void send_toggle_event(void)
{
    (void)xTaskNotifyGive(led_task);
}

static void button_task(void *arg)
{
    (void)arg;
    press_edge_t edge;
    press_edge_init(&edge, board_button_is_pressed(0U));
    TickType_t last = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PRESS_SAMPLE_MS));
        if (press_edge_sample(&edge, board_button_is_pressed(0U)))
        {
            printf("EVT btn1=pressed name=%s pin=%s\r\n", board_button_name(0U), board_button_pin(0U));
            send_toggle_event();
        }
    }
}

static void led_task_fn(void *arg)
{
    (void)arg;
    for (;;)
    {
        /* wait for an event, at most one interval; timeout = time to toggle */
        uint32_t events = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(blink_rate_ms()));
        if (events == 0U)
        {
            (void)led_toggle(1);
            printf("BLINK led1=%d\r\n", led_get(1) ? 1 : 0);
        }
        else
        {
            uint32_t ms = 0U;
            for (uint32_t i = 0U; i < events; i++)
            {
                ms = blink_rate_step();
            }
            printf("PERIOD ms=%lu\r\n", (unsigned long)ms);
        }
    }
}

static void cmd_info(int argc, char *argv[])     { (void)argc; (void)argv; print_info(); }
static void cmd_period_q(int argc, char *argv[]) { (void)argc; (void)argv; printf("PERIOD ms=%lu\r\n", (unsigned long)blink_rate_ms()); }
static void cmd_step(int argc, char *argv[])     { (void)argc; (void)argv; send_toggle_event(); }

static const console_cmd_t commands[] =
{
    { "info",    "info                  - app/board info",          cmd_info     },
    { "period?", "period?               - LED toggle interval",     cmd_period_q },
    { "step",    "step                  - same event as a B1 press", cmd_step    },
    APP_RTOS_CMD_TASKS,
};

void app_main(void)
{
    board_init();
    led_init();
    blink_rate_init();

    printf("\r\n[APP] %s %s on %s (FreeRTOS)\r\n", APP_NAME, APP_VERSION, BOARD_NAME);
    print_info();

    if (board_button_count() == 0U)
    {
        printf("ERR no button on this board - only 'step' sends events\r\n");
    }
    led_task = app_rtos_task(led_task_fn, "ledTask", APP_RTOS_STACK_PRINTF, PRIO_LED, NULL);
    if (board_button_count() > 0U)
    {
        (void)app_rtos_task(button_task, "buttonTask", APP_RTOS_STACK_PRINTF, PRIO_BUTTON, NULL);
    }
    app_rtos_console_start(commands, sizeof(commands) / sizeof(commands[0]));
    app_rtos_run();
}
