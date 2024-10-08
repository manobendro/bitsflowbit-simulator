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

#include <math.h>
#include "py/runtime.h"
#include "py/mphal.h"
#include "modbitsflow.h"

#define GESTURE_LIST_SIZE (8)

typedef struct _bitsflow_accelerometer_obj_t {
    mp_obj_base_t base;
} bitsflow_accelerometer_obj_t;

volatile bool accelerometer_up_to_date = false;
STATIC volatile uint16_t gesture_state = 0;                    // 1 bit per gesture
STATIC volatile uint8_t gesture_list_cur = 0;                  // index into gesture_list
STATIC volatile uint8_t gesture_list[GESTURE_LIST_SIZE] = {0}; // list of pending gestures, 4-bits per element

STATIC const qstr gesture_name_map[] = {
    [BITSFLOW_HAL_ACCELEROMETER_EVT_NONE] = MP_QSTR_,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_UP] = MP_QSTR_up,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_DOWN] = MP_QSTR_down,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_LEFT] = MP_QSTR_left,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_RIGHT] = MP_QSTR_right,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_UP] = MP_QSTR_face_space_up,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_DOWN] = MP_QSTR_face_space_down,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_FREEFALL] = MP_QSTR_freefall,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_2G] = MP_QSTR_2g,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_3G] = MP_QSTR_3g,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_6G] = MP_QSTR_6g,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_8G] = MP_QSTR_8g,
    [BITSFLOW_HAL_ACCELEROMETER_EVT_SHAKE] = MP_QSTR_shake,
};

STATIC uint32_t gesture_from_obj(mp_obj_t gesture_in) {
    qstr gesture = mp_obj_str_get_qstr(gesture_in);
    for (uint i = 0; i < MP_ARRAY_SIZE(gesture_name_map); ++i) {
        if (gesture == gesture_name_map[i]) {
            return i;
        }
    }
    mp_raise_ValueError(MP_ERROR_TEXT("invalid gesture"));
}

STATIC void update_for_gesture(void) {
    if (!accelerometer_up_to_date) {
        accelerometer_up_to_date = true;
        int axis[3];
        bitsflow_hal_accelerometer_get_sample(axis);
    }
}

void bitsflow_hal_gesture_callback(int value) {
    if (value > BITSFLOW_HAL_ACCELEROMETER_EVT_NONE && value <= BITSFLOW_HAL_ACCELEROMETER_EVT_2G) {
        gesture_state |= 1 << value;
        if (gesture_list_cur < 2 * GESTURE_LIST_SIZE) {
            uint8_t entry = gesture_list[gesture_list_cur >> 1];
            if (gesture_list_cur & 1) {
                entry = (entry & 0x0f) | value << 4;
            } else {
                entry = (entry & 0xf0) | value;
            }
            gesture_list[gesture_list_cur >> 1] = entry;
            ++gesture_list_cur;
        }
    }
}

STATIC mp_obj_t bitsflow_accelerometer_get_x(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    bitsflow_hal_accelerometer_get_sample(axis);
    return mp_obj_new_int(axis[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_get_x_obj, bitsflow_accelerometer_get_x);

STATIC mp_obj_t bitsflow_accelerometer_get_y(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    bitsflow_hal_accelerometer_get_sample(axis);
    return mp_obj_new_int(axis[1]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_get_y_obj, bitsflow_accelerometer_get_y);

STATIC mp_obj_t bitsflow_accelerometer_get_z(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    bitsflow_hal_accelerometer_get_sample(axis);
    return mp_obj_new_int(axis[2]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_get_z_obj, bitsflow_accelerometer_get_z);

STATIC mp_obj_t bitsflow_accelerometer_get_values(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    bitsflow_hal_accelerometer_get_sample(axis);
    mp_obj_t tuple[3] = {
        mp_obj_new_int(axis[0]),
        mp_obj_new_int(axis[1]),
        mp_obj_new_int(axis[2]),
    };
    return mp_obj_new_tuple(3, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_get_values_obj, bitsflow_accelerometer_get_values);

STATIC mp_obj_t bitsflow_accelerometer_get_strength(mp_obj_t self_in) {
    (void)self_in;
    int axis[3];
    bitsflow_hal_accelerometer_get_sample(axis);
    int strength = sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    return mp_obj_new_int(strength);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_get_strength_obj, bitsflow_accelerometer_get_strength);

STATIC mp_obj_t bitsflow_accelerometer_current_gesture(mp_obj_t self_in) {
    (void)self_in;
    update_for_gesture();
    return MP_OBJ_NEW_QSTR(gesture_name_map[bitsflow_hal_accelerometer_get_gesture()]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_current_gesture_obj, bitsflow_accelerometer_current_gesture);

STATIC mp_obj_t bitsflow_accelerometer_is_gesture(mp_obj_t self_in, mp_obj_t gesture_in) {
    (void)self_in;
    uint32_t gesture = gesture_from_obj(gesture_in);
    update_for_gesture();
    return mp_obj_new_bool(bitsflow_hal_accelerometer_get_gesture() == gesture);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_accelerometer_is_gesture_obj, bitsflow_accelerometer_is_gesture);

STATIC mp_obj_t bitsflow_accelerometer_was_gesture(mp_obj_t self_in, mp_obj_t gesture_in) {
    (void)self_in;
    uint32_t gesture = gesture_from_obj(gesture_in);
    update_for_gesture();
    mp_obj_t result = mp_obj_new_bool(gesture_state & (1 << gesture));
    gesture_state &= (~(1 << gesture));
    gesture_list_cur = 0;
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_accelerometer_was_gesture_obj, bitsflow_accelerometer_was_gesture);

STATIC mp_obj_t bitsflow_accelerometer_get_gestures(mp_obj_t self_in) {
    (void)self_in;
    update_for_gesture();
    if (gesture_list_cur == 0) {
        return mp_const_empty_tuple;
    }
    mp_obj_tuple_t *o = (mp_obj_tuple_t*)mp_obj_new_tuple(gesture_list_cur, NULL);
    for (uint i = 0; i < gesture_list_cur; ++i) {
        uint gesture = (gesture_list[i >> 1] >> (4 * (i & 1))) & 0x0f;
        o->items[i] = MP_OBJ_NEW_QSTR(gesture_name_map[gesture]);
    }
    gesture_list_cur = 0;
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_accelerometer_get_gestures_obj, bitsflow_accelerometer_get_gestures);

STATIC mp_obj_t bitsflow_accelerometer_set_range(mp_obj_t self_in, mp_obj_t g) {
    (void)self_in;
    bitsflow_hal_accelerometer_set_range(mp_obj_get_int(g));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_accelerometer_set_range_obj, bitsflow_accelerometer_set_range);

STATIC const mp_rom_map_elem_t bitsflow_accelerometer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_x), MP_ROM_PTR(&bitsflow_accelerometer_get_x_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_y), MP_ROM_PTR(&bitsflow_accelerometer_get_y_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_z), MP_ROM_PTR(&bitsflow_accelerometer_get_z_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_values), MP_ROM_PTR(&bitsflow_accelerometer_get_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_strength), MP_ROM_PTR(&bitsflow_accelerometer_get_strength_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_gesture), MP_ROM_PTR(&bitsflow_accelerometer_current_gesture_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_gesture), MP_ROM_PTR(&bitsflow_accelerometer_is_gesture_obj) },
    { MP_ROM_QSTR(MP_QSTR_was_gesture), MP_ROM_PTR(&bitsflow_accelerometer_was_gesture_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gestures), MP_ROM_PTR(&bitsflow_accelerometer_get_gestures_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_range), MP_ROM_PTR(&bitsflow_accelerometer_set_range_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_accelerometer_locals_dict, bitsflow_accelerometer_locals_dict_table);

const mp_obj_type_t bitsflow_accelerometer_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowAccelerometer,
    .locals_dict = (mp_obj_dict_t *)&bitsflow_accelerometer_locals_dict,
};

const bitsflow_accelerometer_obj_t bitsflow_accelerometer_obj = {
    { &bitsflow_accelerometer_type },
};
