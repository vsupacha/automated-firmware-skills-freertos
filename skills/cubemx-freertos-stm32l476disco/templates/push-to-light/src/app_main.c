/*******************************************************************************
* File Name : app_main.c
*
* Layer 3 - execution (FreeRTOS). Wires the function layer together; no HAL/LL
* calls or pins here (those belong in board/).
*
* push-to-light demo (same console contract as the other skills' template):
*   - console task: commands over the ST-LINK virtual COM port (see help)
*   - buttonTask:   BTNn press toggles LEDn and reports an EVT line
*                   (STM32L476G-DISCO: joystick center -> LD4); button_poll() runs every BUTTON_POLL_MS
* Two tasks print: every report line is built first and printed with ONE printf,
* so lines from the two tasks never mix (board/ locks the console per write).
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_main.h"
#include "app_rtos.h"
#include "board.h"
#include "button.h"
#include "console.h"
#include "led.h"

#define APP_NAME        "push-to-light"
#define APP_VERSION     "1.0"
#define PRIO_BUTTON     (tskIDLE_PRIORITY + 3U)

static void print_info(void)
{
    printf("INFO app=%s v=%s board=%s leds=%lu btns=%lu rtos=FreeRTOS %s\r\n", APP_NAME, APP_VERSION,
           BOARD_NAME, (unsigned long)led_count(), (unsigned long)button_count(), tskKERNEL_VERSION_NUMBER);
    printf("READY\r\n");
}

/* "LED led1=x led2=y" or "BTN btn1=x": built in a buffer, printed at once */
static void print_states(const char *tag, const char *item, uint32_t count, bool (*get)(uint32_t))
{
    char   line[80];
    size_t len = (size_t)snprintf(line, sizeof(line), "%s", tag);
    for (uint32_t n = 1U; (n <= count) && (len < sizeof(line)); n++)
    {
        len += (size_t)snprintf(&line[len], sizeof(line) - len, " %s%lu=%d", item, (unsigned long)n,
                                get(n) ? 1 : 0);
    }
    printf("%s\r\n", line);
}

static void print_leds(void)    { print_states("LED", "led", led_count(), led_get); }
static void print_buttons(void) { print_states("BTN", "btn", button_count(), button_is_pressed); }

/* led <n> on|off|toggle */
static void cmd_led(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("ERR usage: led <n> on|off|toggle\r\n");
        return;
    }
    uint32_t n  = (uint32_t)strtoul(argv[1], NULL, 10);
    bool     ok;
    if (strcmp(argv[2], "on") == 0)          { ok = led_set(n, true); }
    else if (strcmp(argv[2], "off") == 0)    { ok = led_set(n, false); }
    else if (strcmp(argv[2], "toggle") == 0) { ok = led_toggle(n); }
    else
    {
        printf("ERR bad action '%s'\r\n", argv[2]);
        return;
    }
    if (ok)
    {
        printf("OK led%lu=%d\r\n", (unsigned long)n, led_get(n) ? 1 : 0);
    }
    else
    {
        printf("ERR no led%s (have %lu)\r\n", argv[1], (unsigned long)led_count());
    }
}

static void cmd_led_q(int argc, char *argv[])  { (void)argc; (void)argv; print_leds(); }
static void cmd_btn_q(int argc, char *argv[])  { (void)argc; (void)argv; print_buttons(); }
static void cmd_info(int argc, char *argv[])   { (void)argc; (void)argv; print_info(); }

static const console_cmd_t commands[] =
{
    { "info", "info                  - app/board info", cmd_info  },
    { "led",  "led <n> on|off|toggle - drive LEDn",     cmd_led   },
    { "led?", "led?                  - read back LEDs", cmd_led_q },
    { "btn?", "btn?                  - button states",  cmd_btn_q },
    APP_RTOS_CMD_TASKS,
};

static void button_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;)
    {
        uint32_t     num;
        button_evt_t evt;

        vTaskDelayUntil(&last, pdMS_TO_TICKS(BUTTON_POLL_MS));
        evt = button_poll(&num);
        if (evt == BUTTON_EVT_PRESSED)
        {
            (void)led_toggle(num);   /* BTNn toggles LEDn when it exists */
            printf("EVT btn%lu=pressed name=%s pin=%s led%lu=%d\r\n", (unsigned long)num,
                   board_button_name(num - 1U), board_button_pin(num - 1U),
                   (unsigned long)num, led_get(num) ? 1 : 0);
        }
        else if (evt == BUTTON_EVT_RELEASED)
        {
            printf("EVT btn%lu=released\r\n", (unsigned long)num);
        }
    }
}

void app_main(void)
{
    board_init();
    led_init();
    button_init();

    printf("\r\n[APP] %s %s on %s (FreeRTOS) - type 'help'\r\n", APP_NAME, APP_VERSION, BOARD_NAME);
    print_info();

    (void)app_rtos_task(button_task, "buttonTask", APP_RTOS_STACK_PRINTF, PRIO_BUTTON, NULL);
    app_rtos_console_start(commands, sizeof(commands) / sizeof(commands[0]));
    app_rtos_run();
}
