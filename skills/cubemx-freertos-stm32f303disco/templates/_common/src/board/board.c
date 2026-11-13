/*******************************************************************************
* File Name : board.c
*
* Layer 1 - board adaptation for STM32F3 boards on the STM32CubeF3 HAL, FreeRTOS
* execution model.
*
* What comes from STM32CubeMX (the app's .ioc, from the CubeMX board
* configuration, regenerated into mx/): clocks, the LED outputs and the B1
* input (MX_GPIO_Init()), SWO on PB3 (SYS: Trace Asynchronous Sw).
* What is done here: the user I/O table, the console glue (_write -> ITM).
*
* FreeRTOS: the HAL tick runs on TIM6 (the .ioc: SYS timebase TIM6), SysTick
* belongs to the kernel. board_delay_ms() blocks the calling task with
* vTaskDelay() once the scheduler runs (busy wait before that and in ISRs);
* _write() holds a mutex while it sends, so lines from different tasks do not
* interleave on SWO.
*
* STM32F3DISCOVERY (STM32CubeF3 BSP Drivers/BSP/STM32F3-Discovery): eight LEDs
* on PE8..PE15 in a ring (LD3 red PE9, LD4 blue PE8, LD5 orange PE10, LD6 green
* PE15, LD7 green PE11, LD8 orange PE14, LD9 blue PE12, LD10 red PE13), all
* active HIGH; B1 PA0 with a pull-down, pressed = HIGH.
*
* Console: printf() goes to ITM stimulus port 0 = SWO through the ST-LINK. The
* debugger configures SWO (core clock = SYSCLK_HZ of the board profile); with
* no SWO viewer attached, ITM output is discarded - an unattended board never
* blocks. There is no console input (the board's ST-LINK/V2 has no VCP).
*******************************************************************************/
#include "board.h"

#include <stdio.h>

#include "main.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          active_high;
    const char   *name;
    const char   *pin_name;
} board_io_t;

#if defined(BOARD_STM32F3_DISCOVERY)
static const board_io_t leds[] =
{
    { GPIOE, GPIO_PIN_9,  true, "LD3",  "PE9"  },      /* red    */
    { GPIOE, GPIO_PIN_8,  true, "LD4",  "PE8"  },      /* blue   */
    { GPIOE, GPIO_PIN_10, true, "LD5",  "PE10" },      /* orange */
    { GPIOE, GPIO_PIN_15, true, "LD6",  "PE15" },      /* green  */
    { GPIOE, GPIO_PIN_11, true, "LD7",  "PE11" },      /* green  */
    { GPIOE, GPIO_PIN_14, true, "LD8",  "PE14" },      /* orange */
    { GPIOE, GPIO_PIN_12, true, "LD9",  "PE12" },      /* blue   */
    { GPIOE, GPIO_PIN_13, true, "LD10", "PE13" },      /* red    */
};
static const board_io_t buttons[] =
{
    { GPIOA, GPIO_PIN_0,  true, "B1",  "PA0"  },
};
#else
#error "board.c: no user I/O table for this board - add a BOARD_xxx block"
#endif

#define N_LEDS      ((uint32_t)(sizeof(leds) / sizeof(leds[0])))
#define N_BUTTONS   ((uint32_t)(sizeof(buttons) / sizeof(buttons[0])))

static SemaphoreHandle_t console_lock;     /* created in board_init(), used once tasks run */

static bool scheduler_running(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

static bool in_isr(void)
{
    return __get_IPSR() != 0U;
}

void board_init(void)
{
    /* pins are configured by the generated MX_GPIO_Init(); start with every LED off */
    for (uint32_t i = 0U; i < N_LEDS; i++)
    {
        board_led_write(i, false);
    }
    console_lock = xSemaphoreCreateMutex();   /* allowed before vTaskStartScheduler() */
    if (console_lock == NULL)
    {
        board_fatal();
    }
}

void board_delay_ms(uint32_t ms)
{
    if (scheduler_running() && !in_isr())
    {
        vTaskDelay(pdMS_TO_TICKS(ms));          /* other tasks run meanwhile */
        return;
    }
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < ms)
    {
    }
}

uint32_t board_millis(void)
{
    return HAL_GetTick();
}

void board_fatal(void)
{
    __disable_irq();
    for (;;)
    {
    }
}

uint32_t board_led_count(void)
{
    return N_LEDS;
}

void board_led_write(uint32_t idx, bool on)
{
    if (idx < N_LEDS)
    {
        bool level = (on == leds[idx].active_high);
        HAL_GPIO_WritePin(leds[idx].port, leds[idx].pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

bool board_led_read(uint32_t idx)
{
    if (idx >= N_LEDS)
    {
        return false;
    }
    /* read back the output data register: the pin really is driven this way */
    bool level = ((leds[idx].port->ODR & leds[idx].pin) != 0U);
    return level == leds[idx].active_high;
}

const char *board_led_name(uint32_t idx)
{
    return (idx < N_LEDS) ? leds[idx].name : "?";
}

uint32_t board_button_count(void)
{
    return N_BUTTONS;
}

bool board_button_is_pressed(uint32_t idx)
{
    if (idx >= N_BUTTONS)
    {
        return false;
    }
    bool level = (HAL_GPIO_ReadPin(buttons[idx].port, buttons[idx].pin) == GPIO_PIN_SET);
    return level == buttons[idx].active_high;
}

const char *board_button_name(uint32_t idx)
{
    return (idx < N_BUTTONS) ? buttons[idx].name : "?";
}

const char *board_button_pin(uint32_t idx)
{
    return (idx < N_BUTTONS) ? buttons[idx].pin_name : "?";
}

/* ---- console: SWO output only ---- */

bool board_console_getc(char *c)
{
    (void)c;
    return false;
}

void board_console_flush(void)
{
}

/* Whole lines per task: stdout is unbuffered, so one printf() reaches _write() in
 * several pieces ("TASK name=", the name, ...). A mutex around each piece is not
 * enough - another task's line could land between two pieces (seen on the board,
 * 2026-11-12: "TASK name=BBLINK led1=1"). Each task's pieces are collected in its
 * own line slot until '\n' and the whole line is sent under console_lock. Before
 * the scheduler runs, in an ISR, or when all slots are taken, bytes go out at once.
 * A line without '\n' (an echoed key) appears when its line ends. */
#define LINE_SLOTS      (8U)
#define LINE_MAX        (160U)

typedef struct
{
    TaskHandle_t owner;
    uint16_t     len;
    char         buf[LINE_MAX];
} line_slot_t;

static line_slot_t line_slots[LINE_SLOTS];

static void console_send(const char *p, uint32_t n)
{
    for (uint32_t i = 0U; i < n; i++)
    {
        (void)ITM_SendChar((uint32_t)(uint8_t)p[i]);
    }
}

static line_slot_t *line_slot_of(TaskHandle_t task)
{
    line_slot_t *free_slot = NULL;
    for (uint32_t i = 0U; i < LINE_SLOTS; i++)
    {
        if (line_slots[i].owner == task)
        {
            return &line_slots[i];
        }
        if ((line_slots[i].owner == NULL) && (free_slot == NULL))
        {
            free_slot = &line_slots[i];
        }
    }
    if (free_slot != NULL)
    {
        free_slot->owner = task;
        free_slot->len   = 0U;
    }
    return free_slot;
}

/* printf() -> ITM port 0 / SWO (overrides the weak _write of the generated syscalls.c) */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (len <= 0)
    {
        return len;
    }
    if ((console_lock == NULL) || !scheduler_running() || in_isr())
    {
        console_send(ptr, (uint32_t)len);
        return len;
    }
    (void)xSemaphoreTake(console_lock, portMAX_DELAY);
    line_slot_t *s = line_slot_of(xTaskGetCurrentTaskHandle());
    if (s == NULL)
    {
        console_send(ptr, (uint32_t)len);
    }
    else
    {
        for (int i = 0; i < len; i++)
        {
            s->buf[s->len++] = ptr[i];
            if ((ptr[i] == '\n') || (s->len == LINE_MAX))
            {
                console_send(s->buf, s->len);
                s->len = 0U;
            }
        }
    }
    (void)xSemaphoreGive(console_lock);
    return len;
}
