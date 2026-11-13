/*******************************************************************************
* File Name : board.c
*
* Layer 1 - board adaptation for STM32L4 boards (FreeRTOS execution model) on the STM32CubeL4 HAL.
*
* What comes from STM32CubeMX (the app's .ioc, from the CubeMX board
* configuration "loadboard ... nomode" + MX_CONFIG, regenerated into mx/):
* clocks, MX_GPIO_Init(), USART2 = huart2 (115200 8N1) on the
* ST-LINK virtual COM port.
* What is done here: the user I/O table (LED and button pins are configured
* here too, so they do not depend on how much of the board configuration
* "nomode" keeps), the console glue (_write -> huart2, blocking TX; RX by the
* RXNE interrupt into a ring buffer).
*
* Why an RX interrupt: the STM32L4 USART has no RX FIFO (one byte in RDR).
* Polling RX while the console echoes with blocking TX overruns it - seen on
* hardware 2026-10-08 ("led 1 off" arrived as "led 1 of"). The handler lives
* here because CubeMX leaves USART2_IRQHandler as the startup file's weak
* default; if USART2 global interrupt is ever enabled in the .ioc (NVIC),
* stm32l4xx_it.c defines it too - keep it disabled there.
*
* STM32L476G-DISCO (STM32CubeL4 BSP Drivers/BSP/STM32L476G-Discovery): LD4 red
* PB2 and LD5 green PE8, active HIGH (BSP_LED_On writes SET); the 5-way
* joystick MT-008A as buttons - center PA0, left PA1, right PA2, up PA3, down
* PA5 - pulled down, pressed = HIGH. Console: USART2 PD5 TX / PD6 RX ->
* ST-LINK/V2-1 VCP.
*******************************************************************************/
#include "board.h"

#include <stdio.h>

#include "main.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

extern UART_HandleTypeDef huart2;       /* generated in mx/Src/main.c (MX_USART2_UART_Init) */

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          active_high;
    const char   *name;
    const char   *pin_name;
} board_io_t;

#if defined(BOARD_STM32L476G_DISCO)
static const board_io_t leds[] =
{
    { GPIOB, GPIO_PIN_2,  true,  "LD4",  "PB2"  },      /* red   */
    { GPIOE, GPIO_PIN_8,  true,  "LD5",  "PE8"  },      /* green */
};
static const board_io_t buttons[] =
{
    { GPIOA, GPIO_PIN_0,  true,  "JOY_CENTER", "PA0" }, /* joystick, pull-down */
    { GPIOA, GPIO_PIN_1,  true,  "JOY_LEFT",   "PA1" },
    { GPIOA, GPIO_PIN_2,  true,  "JOY_RIGHT",  "PA2" },
    { GPIOA, GPIO_PIN_3,  true,  "JOY_UP",     "PA3" },
    { GPIOA, GPIO_PIN_5,  true,  "JOY_DOWN",   "PA5" },
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

static void enable_port_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)      { __HAL_RCC_GPIOA_CLK_ENABLE(); }
    else if (port == GPIOB) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
    else if (port == GPIOC) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
    else if (port == GPIOE) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
    else                    { /* add ports used by other boards here */ }
}

void board_init(void)
{
    GPIO_InitTypeDef cfg = { 0 };

    for (uint32_t i = 0U; i < N_LEDS; i++)
    {
        enable_port_clock(leds[i].port);
        board_led_write(i, false);                       /* level before the pin drives */
        cfg.Pin   = leds[i].pin;
        cfg.Mode  = GPIO_MODE_OUTPUT_PP;
        cfg.Pull  = GPIO_NOPULL;
        cfg.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(leds[i].port, &cfg);
    }
    for (uint32_t i = 0U; i < N_BUTTONS; i++)
    {
        enable_port_clock(buttons[i].port);
        cfg.Pin   = buttons[i].pin;
        cfg.Mode  = GPIO_MODE_INPUT;
        cfg.Pull  = buttons[i].active_high ? GPIO_PULLDOWN : GPIO_PULLUP;
        cfg.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(buttons[i].port, &cfg);
    }
    (void)setvbuf(stdout, NULL, _IONBF, 0);   /* every printf goes out immediately */

    /* console RX: RXNE interrupt -> rx_buf (see USART2_IRQHandler) */
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_PEF);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART2_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(USART2_IRQn);         /* handler calls no RTOS API: any priority works */
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

/* ---- console: USART2 on the ST-LINK virtual COM port ---- */

#define RX_BUF_SIZE     (256U)                  /* power of two */
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint32_t rx_head;               /* written by the ISR  */
static volatile uint32_t rx_tail;               /* written by getc()   */

/* overrides the startup file's weak default (USART2 NVIC is off in the .ioc) */
void USART2_IRQHandler(void)
{
    USART_TypeDef *u = huart2.Instance;
    uint32_t isr = u->ISR;

    if ((isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) != 0U)
    {
        u->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
    }
    if ((isr & USART_ISR_RXNE) != 0U)
    {
        uint8_t  b    = (uint8_t)(u->RDR & 0xFFU);  /* reading RDR clears RXNE */
        uint32_t next = (rx_head + 1U) & (RX_BUF_SIZE - 1U);
        if (next != rx_tail)                        /* full: drop the byte */
        {
            rx_buf[rx_head] = b;
            rx_head = next;
        }
    }
}

bool board_console_getc(char *c)
{
    if (rx_tail == rx_head)
    {
        return false;
    }
    *c = (char)rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) & (RX_BUF_SIZE - 1U);
    return true;
}

void board_console_flush(void)
{
    /* HAL_UART_Transmit() in _write() blocks until the data is sent */
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
    (void)HAL_UART_Transmit(&huart2, (const uint8_t *)p, (uint16_t)n, 100U);
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

/* printf() -> USART2 (overrides the weak _write of the generated syscalls.c) */
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
