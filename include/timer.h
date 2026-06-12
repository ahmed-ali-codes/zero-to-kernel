/* ============================================================================
 * include/timer.h — Programmable Interval Timer (PIT) Interface
 *
 * Purpose : Sets up the PIT to fire interrupts on IRQ0 at a specific
 *           frequency (e.g., 100 Hz). Tracks global uptime ticks.
 * ============================================================================ */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialize the timer at the specified frequency (e.g., 100 Hz) */
void timer_install(uint32_t frequency);

/* Get the total number of timer ticks since boot */
uint32_t timer_get_ticks(void);

/* Get the system uptime in seconds */
uint32_t timer_get_uptime_seconds(void);

#endif /* TIMER_H */
