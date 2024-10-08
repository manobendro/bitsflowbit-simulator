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

#include "py/runtime.h"
#include "modbitsflow.h"

struct _bitsflow_soundevent_obj_t {
    mp_obj_base_t base;
    qstr name;
};

const bitsflow_soundevent_obj_t bitsflow_soundevent_loud_obj = {
    { &bitsflow_soundevent_type },
    MP_QSTR_loud,
};

const bitsflow_soundevent_obj_t bitsflow_soundevent_quiet_obj = {
    { &bitsflow_soundevent_type },
    MP_QSTR_quiet,
};

STATIC void bitsflow_soundevent_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    bitsflow_soundevent_obj_t *self = (bitsflow_soundevent_obj_t *)self_in;
    mp_printf(print, "SoundEvent('%q')", self->name);
}

STATIC const mp_rom_map_elem_t bitsflow_soundevent_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_LOUD), MP_ROM_PTR(&bitsflow_soundevent_loud_obj) },
    { MP_ROM_QSTR(MP_QSTR_QUIET), MP_ROM_PTR(&bitsflow_soundevent_quiet_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_soundevent_locals_dict, bitsflow_soundevent_locals_dict_table);

const mp_obj_type_t bitsflow_soundevent_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowSoundEvent,
    .print = bitsflow_soundevent_print,
    .locals_dict = (mp_obj_dict_t *)&bitsflow_soundevent_locals_dict,
};
