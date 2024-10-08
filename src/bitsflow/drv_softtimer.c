/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mphal.h"
#include "drv_softtimer.h"

#define TICKS_PERIOD 0x80000000
#define TICKS_DIFF(t1, t0) ((int32_t)(((t1 - t0 + TICKS_PERIOD / 2) & (TICKS_PERIOD - 1)) - TICKS_PERIOD / 2))

static bool bitsflow_soft_timer_paused = false;

STATIC int bitsflow_soft_timer_lt(mp_pairheap_t *n1, mp_pairheap_t *n2) {
    bitsflow_soft_timer_entry_t *e1 = (bitsflow_soft_timer_entry_t *)n1;
    bitsflow_soft_timer_entry_t *e2 = (bitsflow_soft_timer_entry_t *)n2;
    return TICKS_DIFF(e1->expiry_ms, e2->expiry_ms) < 0;
}

void bitsflow_soft_timer_deinit(void) {
    MP_STATE_PORT(soft_timer_heap) = NULL;
    bitsflow_soft_timer_paused = false;
}

static void bitsflow_soft_timer_handler_run(bool run_callbacks) {
    uint32_t ticks_ms = mp_hal_ticks_ms();
    bitsflow_soft_timer_entry_t *heap = MP_STATE_PORT(soft_timer_heap);
    while (heap != NULL && TICKS_DIFF(heap->expiry_ms, ticks_ms) <= 0) {
        bitsflow_soft_timer_entry_t *entry = heap;
        heap = (bitsflow_soft_timer_entry_t *)mp_pairheap_pop(bitsflow_soft_timer_lt, &heap->pairheap);
        if (run_callbacks) {
            if (entry->flags & BITSFLOW_SOFT_TIMER_FLAG_PY_CALLBACK) {
                mp_sched_schedule(entry->py_callback, MP_OBJ_FROM_PTR(entry));
            } else {
                entry->c_callback(entry);
            }
        }
        if (entry->mode == BITSFLOW_SOFT_TIMER_MODE_PERIODIC) {
            entry->expiry_ms += entry->delta_ms;
            heap = (bitsflow_soft_timer_entry_t *)mp_pairheap_push(bitsflow_soft_timer_lt, &heap->pairheap, &entry->pairheap);
        }
    }
    MP_STATE_PORT(soft_timer_heap) = heap;
}

// This function can be executed at interrupt priority.
void bitsflow_soft_timer_handler(void) {
    if (!bitsflow_soft_timer_paused) {
        bitsflow_soft_timer_handler_run(true);
    }
}

void bitsflow_soft_timer_insert(bitsflow_soft_timer_entry_t *entry, uint32_t initial_delta_ms) {
    mp_pairheap_init_node(bitsflow_soft_timer_lt, &entry->pairheap);
    entry->expiry_ms = mp_hal_ticks_ms() + initial_delta_ms;
    uint32_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
    MP_STATE_PORT(soft_timer_heap) = (bitsflow_soft_timer_entry_t *)mp_pairheap_push(bitsflow_soft_timer_lt, &MP_STATE_PORT(soft_timer_heap)->pairheap, &entry->pairheap);
    MICROPY_END_ATOMIC_SECTION(atomic_state);
}

void bitsflow_soft_timer_set_pause(bool paused, bool run_callbacks) {
    if (bitsflow_soft_timer_paused && !paused) {
        // Explicitly run the soft timer before unpausing, to catch up on any queued events.
        bitsflow_soft_timer_handler_run(run_callbacks);
    }
    bitsflow_soft_timer_paused = paused;
}

uint32_t bitsflow_soft_timer_get_ms_to_next_expiry(void) {
    bitsflow_soft_timer_entry_t *heap = MP_STATE_PORT(soft_timer_heap);
    if (heap == NULL) {
        return UINT32_MAX;
    }
    int32_t dt = TICKS_DIFF(heap->expiry_ms, mp_hal_ticks_ms());
    if (dt <= 0) {
        return 0;
    }
    return dt;
}
