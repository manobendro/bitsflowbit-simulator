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
#include "py/mphal.h"
#include "modbitsflow.h"

const bitsflow_pin_obj_t bitsflow_p0_obj  = {{&bitsflow_touch_pin_type}, 0, BITSFLOW_HAL_PIN_P0,  MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p1_obj  = {{&bitsflow_touch_pin_type}, 1, BITSFLOW_HAL_PIN_P1,  MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p2_obj  = {{&bitsflow_touch_pin_type}, 2, BITSFLOW_HAL_PIN_P2,  MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p3_obj  = {{&bitsflow_ad_pin_type},    3, BITSFLOW_HAL_PIN_P3,  MODE_DISPLAY};
const bitsflow_pin_obj_t bitsflow_p4_obj  = {{&bitsflow_ad_pin_type},    4, BITSFLOW_HAL_PIN_P4,  MODE_DISPLAY};
const bitsflow_pin_obj_t bitsflow_p5_obj  = {{&bitsflow_dig_pin_type},   5, BITSFLOW_HAL_PIN_P5,  MODE_BUTTON};
const bitsflow_pin_obj_t bitsflow_p6_obj  = {{&bitsflow_dig_pin_type},   6, BITSFLOW_HAL_PIN_P6,  MODE_DISPLAY};
const bitsflow_pin_obj_t bitsflow_p7_obj  = {{&bitsflow_dig_pin_type},   7, BITSFLOW_HAL_PIN_P7,  MODE_DISPLAY};
const bitsflow_pin_obj_t bitsflow_p8_obj  = {{&bitsflow_dig_pin_type},   8, BITSFLOW_HAL_PIN_P8,  MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p9_obj  = {{&bitsflow_dig_pin_type},   9, BITSFLOW_HAL_PIN_P9,  MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p10_obj = {{&bitsflow_ad_pin_type},   10, BITSFLOW_HAL_PIN_P10, MODE_DISPLAY};
const bitsflow_pin_obj_t bitsflow_p11_obj = {{&bitsflow_dig_pin_type},  11, BITSFLOW_HAL_PIN_P11, MODE_BUTTON};
const bitsflow_pin_obj_t bitsflow_p12_obj = {{&bitsflow_dig_pin_type},  12, BITSFLOW_HAL_PIN_P12, MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p13_obj = {{&bitsflow_dig_pin_type},  13, BITSFLOW_HAL_PIN_P13, MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p14_obj = {{&bitsflow_dig_pin_type},  14, BITSFLOW_HAL_PIN_P14, MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p15_obj = {{&bitsflow_dig_pin_type},  15, BITSFLOW_HAL_PIN_P15, MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p16_obj = {{&bitsflow_dig_pin_type},  16, BITSFLOW_HAL_PIN_P16, MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_p19_obj = {{&bitsflow_dig_pin_type},  19, BITSFLOW_HAL_PIN_P19, MODE_I2C};
const bitsflow_pin_obj_t bitsflow_p20_obj = {{&bitsflow_dig_pin_type},  20, BITSFLOW_HAL_PIN_P20, MODE_I2C};

const bitsflow_pin_obj_t bitsflow_pin_logo_obj = {{&bitsflow_touch_only_pin_type}, 30, BITSFLOW_HAL_PIN_FACE, MODE_UNUSED};
const bitsflow_pin_obj_t bitsflow_pin_speaker_obj = {{&bitsflow_dig_pin_type}, 31, BITSFLOW_HAL_PIN_SPEAKER, MODE_UNUSED};

static mp_obj_t bitsflow_pin_get_mode_func(mp_obj_t self_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    return MP_OBJ_NEW_QSTR(bitsflow_pin_get_mode(self)->name);
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_pin_get_mode_obj, bitsflow_pin_get_mode_func);

mp_obj_t bitsflow_pin_write_digital(mp_obj_t self_in, mp_obj_t value_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    int val = mp_obj_get_int(value_in);
    if (val >> 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("value must be 0 or 1"));
    }
    if (bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_write_digital)) {
    }
    bitsflow_hal_pin_write(self->name, val);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_pin_write_digital_obj, bitsflow_pin_write_digital);

mp_obj_t bitsflow_pin_read_digital(mp_obj_t self_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    if (bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_read_digital)) {
    }
    return mp_obj_new_int(bitsflow_hal_pin_read(self->name));
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_pin_read_digital_obj, bitsflow_pin_read_digital);

mp_obj_t bitsflow_pin_set_pull(mp_obj_t self_in, mp_obj_t pull_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    int pull = mp_obj_get_int(pull_in);
    // Pull only applies in an read digital mode
    bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_read_digital);
    bitsflow_hal_pin_set_pull(self->name, pull);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_pin_set_pull_obj, bitsflow_pin_set_pull);

mp_obj_t bitsflow_pin_get_pull(mp_obj_t self_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    const bitsflow_pinmode_t *mode = bitsflow_pin_get_mode(self);
    // Pull only applies in an read digital mode (and button mode behaves like that too)
    if (mode != bitsflow_pin_mode_read_digital && mode != bitsflow_pin_mode_button) {
        pinmode_error(self);
    }
    return mp_obj_new_int(bitsflow_hal_pin_get_pull(self->name));
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_pin_get_pull_obj, bitsflow_pin_get_pull);

mp_obj_t bitsflow_pin_write_analog(mp_obj_t self_in, mp_obj_t value_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    int set_value;
    if (mp_obj_is_float(value_in)) {
        mp_float_t val = mp_obj_get_float(value_in);
        set_value = val + 0.5;
    } else {
        set_value = mp_obj_get_int(value_in);
    }
    if (set_value < 0 || set_value > 1023) {
        mp_raise_ValueError(MP_ERROR_TEXT("value must be between 0 and 1023"));
    }
    if (bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_write_analog)) {
    }
    bitsflow_hal_pin_write_analog_u10(self->name, set_value);
    if (set_value == 0) {
        bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_unused);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_pin_write_analog_obj, bitsflow_pin_write_analog);

mp_obj_t bitsflow_pin_read_analog(mp_obj_t self_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_unused);
    return mp_obj_new_int(bitsflow_hal_pin_read_analog_u10(self->name));
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_pin_read_analog_obj, bitsflow_pin_read_analog);

mp_obj_t bitsflow_pin_set_analog_period(mp_obj_t self_in, mp_obj_t period_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    mp_int_t period = mp_obj_get_int(period_in) * 1000;
    if (bitsflow_hal_pin_set_analog_period_us(self->name, period) == -1) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid period"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_pin_set_analog_period_obj, bitsflow_pin_set_analog_period);

mp_obj_t bitsflow_pin_set_analog_period_microseconds(mp_obj_t self_in, mp_obj_t period_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    mp_int_t period = mp_obj_get_int(period_in);
    if (bitsflow_hal_pin_set_analog_period_us(self->name, period) == -1) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid period"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_pin_set_analog_period_microseconds_obj, bitsflow_pin_set_analog_period_microseconds);

mp_obj_t bitsflow_pin_get_analog_period_microseconds(mp_obj_t self_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    mp_int_t period = bitsflow_hal_pin_get_analog_period_us(self->name);
    return mp_obj_new_int(period);
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_pin_get_analog_period_microseconds_obj, bitsflow_pin_get_analog_period_microseconds);

mp_obj_t bitsflow_pin_is_touched(mp_obj_t self_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t*)self_in;
    const bitsflow_pinmode_t *mode = bitsflow_pin_get_mode(self);
    if (mode != bitsflow_pin_mode_touch && mode != bitsflow_pin_mode_button) {
        bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_touch);
        // set NO_PULL on pin
    }
    return mp_obj_new_bool(bitsflow_hal_pin_is_touched(self->name));
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_pin_is_touched_obj, bitsflow_pin_is_touched);

mp_obj_t bitsflow_pin_set_touch_mode(mp_obj_t self_in, mp_obj_t mode_in) {
    bitsflow_pin_obj_t *self = (bitsflow_pin_obj_t *)self_in;
    const bitsflow_pinmode_t *mode = bitsflow_pin_get_mode(self);
    if (mode != bitsflow_pin_mode_touch && mode != bitsflow_pin_mode_button) {
        bitsflow_obj_pin_acquire(self, bitsflow_pin_mode_touch);
    }
    bitsflow_hal_pin_set_touch_mode(self->name, mp_obj_get_int(mode_in));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_pin_set_touch_mode_obj, bitsflow_pin_set_touch_mode);

#define PULL_CONSTANTS \
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(BITSFLOW_HAL_PIN_PULL_UP) }, \
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(BITSFLOW_HAL_PIN_PULL_DOWN) }, \
    { MP_ROM_QSTR(MP_QSTR_NO_PULL), MP_ROM_INT(BITSFLOW_HAL_PIN_PULL_NONE) }

#define TOUCH_CONSTANTS \
    { MP_ROM_QSTR(MP_QSTR_RESISTIVE), MP_ROM_INT(BITSFLOW_HAL_PIN_TOUCH_RESISTIVE) }, \
    { MP_ROM_QSTR(MP_QSTR_CAPACITIVE), MP_ROM_INT(BITSFLOW_HAL_PIN_TOUCH_CAPACITIVE) }

STATIC const mp_rom_map_elem_t bitsflow_dig_pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write_digital), MP_ROM_PTR(&bitsflow_pin_write_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_digital), MP_ROM_PTR(&bitsflow_pin_read_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_analog), MP_ROM_PTR(&bitsflow_pin_write_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_analog_period), MP_ROM_PTR(&bitsflow_pin_set_analog_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_analog_period_microseconds), MP_ROM_PTR(&bitsflow_pin_set_analog_period_microseconds_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_analog_period_microseconds), MP_ROM_PTR(&bitsflow_pin_get_analog_period_microseconds_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pull), MP_ROM_PTR(&bitsflow_pin_get_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pull), MP_ROM_PTR(&bitsflow_pin_set_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mode), MP_ROM_PTR(&bitsflow_pin_get_mode_obj) },
    PULL_CONSTANTS,
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_dig_pin_locals_dict, bitsflow_dig_pin_locals_dict_table);

const mp_obj_type_t bitsflow_dig_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowDigitalPin,
    .locals_dict = (mp_obj_dict_t *)&bitsflow_dig_pin_locals_dict,
};

STATIC const mp_rom_map_elem_t bitsflow_ann_pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write_digital), MP_ROM_PTR(&bitsflow_pin_write_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_digital), MP_ROM_PTR(&bitsflow_pin_read_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_analog), MP_ROM_PTR(&bitsflow_pin_write_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_analog), MP_ROM_PTR(&bitsflow_pin_read_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_analog_period), MP_ROM_PTR(&bitsflow_pin_set_analog_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_analog_period_microseconds), MP_ROM_PTR(&bitsflow_pin_set_analog_period_microseconds_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_analog_period_microseconds), MP_ROM_PTR(&bitsflow_pin_get_analog_period_microseconds_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pull), MP_ROM_PTR(&bitsflow_pin_get_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pull), MP_ROM_PTR(&bitsflow_pin_set_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mode), MP_ROM_PTR(&bitsflow_pin_get_mode_obj) },
    PULL_CONSTANTS,
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_ann_pin_locals_dict, bitsflow_ann_pin_locals_dict_table);

const mp_obj_type_t bitsflow_ad_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowAnalogDigitalPin,
    .locals_dict = (mp_obj_dict_t *)&bitsflow_ann_pin_locals_dict,
};

STATIC const mp_rom_map_elem_t bitsflow_touch_pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write_digital), MP_ROM_PTR(&bitsflow_pin_write_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_digital), MP_ROM_PTR(&bitsflow_pin_read_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_analog), MP_ROM_PTR(&bitsflow_pin_write_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_analog), MP_ROM_PTR(&bitsflow_pin_read_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_analog_period), MP_ROM_PTR(&bitsflow_pin_set_analog_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_analog_period_microseconds), MP_ROM_PTR(&bitsflow_pin_set_analog_period_microseconds_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_analog_period_microseconds), MP_ROM_PTR(&bitsflow_pin_get_analog_period_microseconds_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_touched), MP_ROM_PTR(&bitsflow_pin_is_touched_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pull), MP_ROM_PTR(&bitsflow_pin_get_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pull), MP_ROM_PTR(&bitsflow_pin_set_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mode), MP_ROM_PTR(&bitsflow_pin_get_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_touch_mode), MP_ROM_PTR(&bitsflow_pin_set_touch_mode_obj) },
    PULL_CONSTANTS,
    TOUCH_CONSTANTS,
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_touch_pin_locals_dict, bitsflow_touch_pin_locals_dict_table);

const mp_obj_type_t bitsflow_touch_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowTouchPin,
    .locals_dict = (mp_obj_dict_t *)&bitsflow_touch_pin_locals_dict,
};

STATIC const mp_rom_map_elem_t bitsflow_touch_only_pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_is_touched), MP_ROM_PTR(&bitsflow_pin_is_touched_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_touch_mode), MP_ROM_PTR(&bitsflow_pin_set_touch_mode_obj) },
    TOUCH_CONSTANTS,
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_touch_only_pin_locals_dict, bitsflow_touch_only_pin_locals_dict_table);

const mp_obj_type_t bitsflow_touch_only_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowTouchOnlyPin,
    .locals_dict = (mp_obj_dict_t *)&bitsflow_touch_only_pin_locals_dict,
};

const bitsflow_pin_obj_t *bitsflow_obj_get_pin(mp_const_obj_t o) {
    const mp_obj_type_t *type = mp_obj_get_type(o);
    if (bitsflow_obj_type_is_pin(type)) {
        return (bitsflow_pin_obj_t*)o;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("expecting a pin"));
    }
}

uint8_t bitsflow_obj_get_pin_name(mp_obj_t o) {
    return bitsflow_obj_get_pin(o)->name;
}
