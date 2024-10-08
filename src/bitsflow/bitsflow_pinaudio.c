/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Damien P. George
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

#include "bitsflowhal.h"
#include "modbitsflow.h"

// The currently selected pin output for the audio.
STATIC const bitsflow_pin_obj_t *audio_routed_pin = NULL;

void bitsflow_pin_audio_select(mp_const_obj_t select, const bitsflow_pinmode_t *pinmode) {
    // Work out which pins are requested for the audio output.
    const bitsflow_pin_obj_t *pin_selected;
    if (select == mp_const_none) {
        pin_selected = NULL;
    } else if (select == MP_OBJ_FROM_PTR(&bitsflow_pin_speaker_obj)) {
        mp_raise_ValueError(MP_ERROR_TEXT("pin_speaker not allowed"));
    } else {
        pin_selected = bitsflow_obj_get_pin((mp_obj_t)select);
    }

    // Change the pin if needed.
    if (pin_selected != audio_routed_pin) {
        if (audio_routed_pin != NULL) {
            bitsflow_obj_pin_free(audio_routed_pin);
        }
        audio_routed_pin = pin_selected;
        if (audio_routed_pin == NULL) {
            bitsflow_hal_audio_select_pin(-1);
        } else {
            bitsflow_obj_pin_acquire(audio_routed_pin, pinmode);
            bitsflow_hal_audio_select_pin(audio_routed_pin->name);
        }
    } else if (audio_routed_pin != NULL) {
        // Update the pin acquisition mode, to make sure pin.get_mode() reflects the current mode.
        bitsflow_pin_set_mode(audio_routed_pin, pinmode);
    }
}

void bitsflow_pin_audio_free(void) {
    if (audio_routed_pin != NULL) {
        bitsflow_obj_pin_free(audio_routed_pin);
        audio_routed_pin = NULL;
        bitsflow_hal_audio_select_pin(-1);
    }
}
