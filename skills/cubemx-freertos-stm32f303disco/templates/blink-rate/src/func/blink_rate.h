/*******************************************************************************
* File Name : blink_rate.h
*
* Layer 2 - logic (portable C, no board or RTOS calls): the blink-rate demo's
* rules, so they can be tested apart from the tasks that run them.
*
*   blink_rate_*      LED toggle interval: 1000 ms -> 500 ms -> 200 ms -> 1000 ms ...
*                     one step per event
*   press_edge_*      press detection on a SAMPLED button (one sample every
*                     PRESS_SAMPLE_MS): an event when a "pressed" sample follows a
*                     "released" one. Sampling slower than contact bounce (< 20 ms)
*                     is the debounce - unlike button_poll(), which needs a call
*                     every BUTTON_POLL_MS.
*******************************************************************************/
#ifndef BLINK_RATE_H
#define BLINK_RATE_H

#include <stdbool.h>
#include <stdint.h>

#define PRESS_SAMPLE_MS     (100U)

void     blink_rate_init(void);        /* back to the first interval (1000 ms) */
uint32_t blink_rate_ms(void);          /* current toggle interval              */
uint32_t blink_rate_step(void);        /* next interval (wraps), returns it    */

typedef struct
{
    bool last;                          /* previous sample: true = pressed */
} press_edge_t;

void press_edge_init(press_edge_t *e, bool pressed_now);
bool press_edge_sample(press_edge_t *e, bool pressed);   /* true on released -> pressed */

#endif /* BLINK_RATE_H */
