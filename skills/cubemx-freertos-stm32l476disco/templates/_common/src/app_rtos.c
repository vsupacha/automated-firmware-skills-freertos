/*******************************************************************************
* File Name : app_rtos.c
*
* Layer 3 - execution layer helpers for the FreeRTOS execution model. See
* app_rtos.h. Uses board.h, func/ and the FreeRTOS kernel API only.
*******************************************************************************/
#include "app_rtos.h"

#include <stdio.h>

#include "board.h"
#include "console.h"

static TaskHandle_t tasks[APP_RTOS_MAX_TASKS];
static uint32_t     n_tasks;

TaskHandle_t app_rtos_task(TaskFunction_t fn, const char *name, uint16_t stack_words,
                           UBaseType_t prio, void *arg)
{
    TaskHandle_t h = NULL;

    if ((n_tasks >= APP_RTOS_MAX_TASKS) ||
        (xTaskCreate(fn, name, stack_words, arg, prio, &h) != pdPASS))
    {
        printf("FATAL cannot create task %s (stack %u words) - heap or APP_RTOS_MAX_TASKS too small\r\n",
               name, (unsigned)stack_words);
        board_fatal();
    }
    tasks[n_tasks++] = h;
    return h;
}

static void console_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;)
    {
        console_poll();                     /* reads every byte waiting, runs complete lines */
        vTaskDelayUntil(&last, pdMS_TO_TICKS(APP_RTOS_CONSOLE_POLL_MS));
    }
}

void app_rtos_console_start(const console_cmd_t *cmds, uint32_t count)
{
    console_init(cmds, count);
    (void)app_rtos_task(console_task, "console", APP_RTOS_STACK_PRINTF, APP_RTOS_PRIO_CONSOLE, NULL);
}

static const char *state_name(eTaskState s)
{
    switch (s)
    {
        case eRunning:   return "running";
        case eReady:     return "ready";
        case eBlocked:   return "blocked";
        case eSuspended: return "suspended";
        case eDeleted:   return "deleted";
        default:         return "?";
    }
}

/* tasks: one line per task, then the heap - "TASK name=ledTask state=blocked prio=2 stack_free=402" */
void app_rtos_cmd_tasks(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    for (uint32_t i = 0U; i < n_tasks; i++)
    {
        printf("TASK name=%s state=%s prio=%lu stack_free=%lu\r\n", pcTaskGetName(tasks[i]),
               state_name(eTaskGetState(tasks[i])), (unsigned long)uxTaskPriorityGet(tasks[i]),
               (unsigned long)uxTaskGetStackHighWaterMark(tasks[i]));
    }
    printf("HEAP free=%lu min=%lu total=%lu tasks=%lu\r\n", (unsigned long)xPortGetFreeHeapSize(),
           (unsigned long)xPortGetMinimumEverFreeHeapSize(), (unsigned long)configTOTAL_HEAP_SIZE,
           (unsigned long)uxTaskGetNumberOfTasks());
}

void app_rtos_run(void)
{
    vTaskStartScheduler();
    printf("FATAL scheduler did not start (heap too small for the idle/timer task)\r\n");
    board_fatal();
}

/* FreeRTOS hooks: the .ioc turns on configCHECK_FOR_STACK_OVERFLOW = 2 and STM32CubeMX
 * then generates vApplicationStackOverflowHook() itself (mx/Src/freertos.c, in a USER
 * CODE section; the overlay makes it stop with interrupts off) - do not define it here
 * (duplicate symbol). A debugger attached after an overflow finds the core in that loop. */
