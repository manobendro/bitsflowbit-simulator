/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2020 Damien P. George
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

#include "py/runtime.h"
#include "modaudio.h"
#include "modbitsflow.h"
#include "modmusic.h"

uint8_t bitsflow_pinmode_indices[32] = { 0 };

const bitsflow_pinmode_t *bitsflow_pin_get_mode(const bitsflow_pin_obj_t *pin) {
    uint8_t pinmode = bitsflow_pinmode_indices[pin->number];
    if (pinmode == 0) {
        pinmode = pin->initial_mode;
    }
    return &bitsflow_pinmodes[pinmode];
}

void bitsflow_pin_set_mode(const bitsflow_pin_obj_t *pin, const bitsflow_pinmode_t *mode) {
    uint32_t index = mode - &bitsflow_pinmodes[0];
    bitsflow_pinmode_indices[pin->number] = index;
    return;
}

void bitsflow_obj_pin_free(const bitsflow_pin_obj_t *pin) {
    if (pin != NULL) {
        bitsflow_pin_set_mode(pin, bitsflow_pin_mode_unused);
    }
}

bool bitsflow_obj_pin_can_be_acquired(const bitsflow_pin_obj_t *pin) {
    const bitsflow_pinmode_t *current_mode = bitsflow_pin_get_mode(pin);
    return current_mode->release != pinmode_error;
}

bool bitsflow_obj_pin_acquire(const bitsflow_pin_obj_t *pin, const bitsflow_pinmode_t *new_mode) {
    const bitsflow_pinmode_t *current_mode = bitsflow_pin_get_mode(pin);

    // The button mode is effectively a digital-in mode, so allow read_digital to work on a button
    if (current_mode == bitsflow_pin_mode_button && new_mode == bitsflow_pin_mode_read_digital) {
        return false;
    }

    if (current_mode != new_mode) {
        current_mode->release(pin);
        bitsflow_pin_set_mode(pin, new_mode);
        return true;
    } else {
        return false;
    }
}

static void noop(const bitsflow_pin_obj_t *pin) {
    (void)pin;
}

void pinmode_error(const bitsflow_pin_obj_t *pin) {
    const bitsflow_pinmode_t *current_mode = bitsflow_pin_get_mode(pin);
    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Pin %d in %q mode"), pin->number, current_mode->name));
}

static void analog_release(const bitsflow_pin_obj_t *pin) {
    // TODO: pwm_release()
}

static void audio_music_release(const bitsflow_pin_obj_t *pin) {
    if (bitsflow_audio_is_playing() || bitsflow_music_is_playing()) {
        pinmode_error(pin);
    } else {
        bitsflow_pin_audio_free();
    }
}

const bitsflow_pinmode_t bitsflow_pinmodes[] = {
    [MODE_UNUSED]        = { MP_QSTR_unused, noop },
    [MODE_WRITE_ANALOG]  = { MP_QSTR_write_analog, analog_release },
    [MODE_READ_DIGITAL]  = { MP_QSTR_read_digital, noop },
    [MODE_WRITE_DIGITAL] = { MP_QSTR_write_digital, noop },
    [MODE_DISPLAY]       = { MP_QSTR_display, pinmode_error },
    [MODE_BUTTON]        = { MP_QSTR_button, pinmode_error },
    [MODE_MUSIC]         = { MP_QSTR_music, audio_music_release },
    [MODE_AUDIO_PLAY]    = { MP_QSTR_audio, audio_music_release },
    [MODE_TOUCH]         = { MP_QSTR_touch, noop },
    [MODE_I2C]           = { MP_QSTR_i2c, pinmode_error },
    [MODE_SPI]           = { MP_QSTR_spi, pinmode_error }
};
