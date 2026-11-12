/*******************************************************************************
* File Name : blink_rate.c
*
* Layer 2 - logic: blink interval steps and sampled-button press detection.
* See blink_rate.h.
*******************************************************************************/
#include "blink_rate.h"

static const uint32_t steps_ms[] = { 1000U, 500U, 200U };
#define N_STEPS     (sizeof(steps_ms) / sizeof(steps_ms[0]))

static uint32_t step;

void blink_rate_init(void)
{
    step = 0U;
}

uint32_t blink_rate_ms(void)
{
    return steps_ms[step];
}

uint32_t blink_rate_step(void)
{
    step = (step + 1U) % (uint32_t)N_STEPS;
    return steps_ms[step];
}

void press_edge_init(press_edge_t *e, bool pressed_now)
{
    e->last = pressed_now;          /* held at start-up: no event until released first */
}

bool press_edge_sample(press_edge_t *e, bool pressed)
{
    bool edge = pressed && !e->last;
    e->last = pressed;
    return edge;
}
