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
#include "py/objstr.h"
#include "py/mphal.h"
#include "drv_system.h"
#include "modbitsflow.h"
#include "modmusic.h"

#define music_data MP_STATE_PORT(music_data)

#define DEFAULT_BPM      (120)
#define DEFAULT_TICKS    (4) // i.e. 4 ticks per beat
#define DEFAULT_OCTAVE   (4) // C4 is middle C
#define DEFAULT_DURATION (4) // Crotchet
#define ARTICULATION_MS  (10) // articulation between notes in milliseconds

#define MUSIC_OUTPUT_DEFAULT_PIN (&bitsflow_pin_default_audio_obj)
#define MUSIC_OUTPUT_AMPLITUDE_OFF (0)
#define MUSIC_OUTPUT_AMPLITUDE_ON (128)

enum {
    ASYNC_MUSIC_STATE_IDLE,
    ASYNC_MUSIC_STATE_NEXT_NOTE,
    ASYNC_MUSIC_STATE_ARTICULATE,
};

typedef struct _music_data_t {
    uint16_t bpm;
    uint16_t ticks;

    // store these to simplify the writing process
    uint8_t last_octave;
    uint8_t last_duration;

    // Asynchronous parts.
    volatile uint8_t async_state;
    bool async_loop;
    uint32_t async_wait_ticks;
    uint16_t async_notes_len;
    uint16_t async_notes_index;
    mp_obj_t async_note;
} music_data_t;

STATIC uint32_t start_note(const char *note_str, size_t note_len);

STATIC void music_output_amplitude(uint32_t amplitude) {
    bitsflow_hal_pin_write_analog_u10(BITSFLOW_HAL_PIN_MIXER, amplitude);
}

STATIC int music_output_period_us(uint32_t period) {
    return bitsflow_hal_pin_set_analog_period_us(BITSFLOW_HAL_PIN_MIXER, period);
}

bool bitsflow_music_is_playing(void) {
    return music_data != NULL && music_data->async_state != ASYNC_MUSIC_STATE_IDLE;
}

// This runs on a hardware interrupt.
void bitsflow_music_tick(void) {
    if (music_data == NULL) {
        // music module not yet imported
        return;
    }

    if (music_data->async_state == ASYNC_MUSIC_STATE_IDLE) {
        // nothing to do
        return;
    }

    if (mp_hal_ticks_ms() < music_data->async_wait_ticks) {
        // need to wait for timeout to expire
        return;
    }

    if (music_data->async_state == ASYNC_MUSIC_STATE_ARTICULATE) {
        // turn off output and rest
        music_output_amplitude(MUSIC_OUTPUT_AMPLITUDE_OFF);
        music_data->async_wait_ticks = mp_hal_ticks_ms() + ARTICULATION_MS;
        music_data->async_state = ASYNC_MUSIC_STATE_NEXT_NOTE;
    } else if (music_data->async_state == ASYNC_MUSIC_STATE_NEXT_NOTE) {
        // play next note
        if (music_data->async_notes_index >= music_data->async_notes_len) {
            if (music_data->async_loop) {
                music_data->async_notes_index = 0;
            } else {
                music_data->async_state = ASYNC_MUSIC_STATE_IDLE;
                return;
            }
        }
        mp_obj_t note;
        if (music_data->async_notes_len == 1) {
            note = music_data->async_note;
        } else {
            note = ((mp_obj_t *)music_data->async_note)[music_data->async_notes_index];
        }
        if (!mp_obj_is_str_or_bytes(note)) {
            mp_printf(&mp_plat_print, "TypeError: expecting a str for note\n");
            music_data->async_state = ASYNC_MUSIC_STATE_IDLE;
            return;
        }
        mp_uint_t note_len;
        const char *note_str = mp_obj_str_get_data(note, &note_len);
        uint32_t delay_on = start_note(note_str, note_len);
        music_data->async_wait_ticks = mp_hal_ticks_ms() + delay_on;
        music_data->async_notes_index += 1;
        music_data->async_state = ASYNC_MUSIC_STATE_ARTICULATE;
    }
}

STATIC void wait_async_music_idle(void) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Wait for the async music state to become idle.
        while (music_data->async_state != ASYNC_MUSIC_STATE_IDLE) {
            mp_handle_pending(true);
            bitsflow_hal_idle();
        }
        nlr_pop();
    } else {
        // Catch all exceptions and stop the music before re-raising.
        music_data->async_state = ASYNC_MUSIC_STATE_IDLE;
        music_output_amplitude(MUSIC_OUTPUT_AMPLITUDE_OFF);
        nlr_jump(nlr.ret_val);
    }
}

STATIC uint32_t start_note(const char *note_str, size_t note_len) {
    music_output_amplitude(MUSIC_OUTPUT_AMPLITUDE_ON);

    // [NOTE](#|b)(octave)(:length)
    // technically, c4 is middle c, so we'll go with that...
    // if we define A as 0 and G as 7, then we can use the following
    // array of us periods

    // these are the periods of note4 (the octave ascending from middle c) from A->B then C->G
    STATIC uint16_t periods_us[] = {2273, 2025, 3822, 3405, 3034, 2863, 2551};
    // A#, -, C#, D#, -, F#, G#
    STATIC uint16_t periods_sharps_us[] = {2145, 0, 3608, 3214, 0, 2703, 2408};

    // we'll represent the note as an integer (A=0, G=6)
    // TODO: validate the note
    uint8_t note_index = (note_str[0] & 0x1f) - 1;

    // TODO: the duration and bpm should be persistent between notes
    uint32_t ms_per_tick = (60000 / music_data->bpm) / music_data->ticks;

    int8_t octave = 0;
    bool sharp = false;

    size_t current_position = 1;

    // parse sharp or flat
    if (current_position < note_len && (note_str[current_position] == '#' || note_str[current_position] == 'b')) {
        if (note_str[current_position] == 'b') {
            // make sure we handle wrapping round gracefully
            if (note_index == 0) {
                note_index = 6;
            } else {
                note_index--;
            }

            // handle the unusual edge case of Cb
            if (note_index == 1) {
                octave--;
            }
        }

        sharp = true;
        current_position++;
    }

    // parse the octave
    if (current_position < note_len && note_str[current_position] != ':') {
        // currently this will only work with a one digit number
        // use +=, since the sharp/flat code changes octave to compensate.
        music_data->last_octave = (note_str[current_position] & 0xf);
        current_position++;
    }

    octave += music_data->last_octave;

    // parse the duration
    if (current_position < note_len && note_str[current_position] == ':') {
        // I'll make this handle up to two digits for the time being.
        current_position++;

        if (current_position < note_len) {
            music_data->last_duration = note_str[current_position] & 0xf;

            current_position++;
            if (current_position < note_len) {
                music_data->last_duration *= 10;
                music_data->last_duration += note_str[current_position] & 0xf;
            }
        } else {
            // technically, this should be a syntax error, since this means
            // that no duration has been specified. For the time being,
            // we'll let you off :D
        }
    }
    // play the note!

    // make the octave relative to octave 4
    octave -= 4;

    // 18 is 'r' or 'R'
    if (note_index < 10) {
        uint32_t period;
        if (sharp) {
            if (octave >= 0) {
                period = periods_sharps_us[note_index] >> octave;
            }
            else {
                period = periods_sharps_us[note_index] << -octave;
            }
        } else {
            if (octave >= 0) {
                period = periods_us[note_index] >> octave;
            }
            else {
                period = periods_us[note_index] << -octave;
            }
        }
        music_output_period_us(period);
    } else {
        music_output_amplitude(MUSIC_OUTPUT_AMPLITUDE_OFF);
    }

    // Cut off a short time from end of note so we hear articulation.
    mp_int_t gap_ms = (ms_per_tick * music_data->last_duration) - ARTICULATION_MS;
    if (gap_ms < ARTICULATION_MS) {
        gap_ms = ARTICULATION_MS;
    }
    return gap_ms;
}

STATIC mp_obj_t bitsflow_music_reset(void) {
    music_data->bpm = DEFAULT_BPM;
    music_data->ticks = DEFAULT_TICKS;
    music_data->last_octave = DEFAULT_OCTAVE;
    music_data->last_duration = DEFAULT_DURATION;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(bitsflow_music_reset_obj, bitsflow_music_reset);

STATIC mp_obj_t bitsflow_music_get_tempo(void) {
    mp_obj_t tempo_tuple[2];
    tempo_tuple[0] = mp_obj_new_int(music_data->bpm);
    tempo_tuple[1] = mp_obj_new_int(music_data->ticks);
    return mp_obj_new_tuple(2, tempo_tuple);
}
MP_DEFINE_CONST_FUN_OBJ_0(bitsflow_music_get_tempo_obj, bitsflow_music_get_tempo);

STATIC mp_obj_t bitsflow_music_stop(mp_uint_t n_args, const mp_obj_t *args) {
    mp_const_obj_t pin;
    if (n_args == 0) {
        pin = MUSIC_OUTPUT_DEFAULT_PIN;
    } else {
        pin = args[0];
    }
    // Raise exception if the pin we are trying to stop is not in a compatible mode.
    bitsflow_pin_audio_select(pin, bitsflow_pin_mode_music);

    // Stop any ongoing background music
    music_data->async_state = ASYNC_MUSIC_STATE_IDLE;

    // Turn off the output
    music_output_amplitude(MUSIC_OUTPUT_AMPLITUDE_OFF);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(bitsflow_music_stop_obj, 0, 1, bitsflow_music_stop);

STATIC mp_obj_t bitsflow_music_play(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_music, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_pin,   MP_ARG_OBJ, {.u_obj = (mp_obj_t)MUSIC_OUTPUT_DEFAULT_PIN} },
        { MP_QSTR_wait,  MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_loop,  MP_ARG_BOOL, {.u_bool = false} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // reset octave and duration so tunes always play the same
    music_data->last_octave = DEFAULT_OCTAVE;
    music_data->last_duration = DEFAULT_DURATION;

    // get either a single note or a list of notes
    mp_uint_t len;
    mp_obj_t *items;
    if (MP_OBJ_IS_STR_OR_BYTES(args[0].u_obj)) {
        len = 1;
        items = &args[0].u_obj;
    } else {
        mp_obj_get_array(args[0].u_obj, &len, &items);
    }

    // Stop any ongoing background music
    music_data->async_state = ASYNC_MUSIC_STATE_IDLE;

    // get the pin to play on
    bitsflow_pin_audio_select(args[1].u_obj, bitsflow_pin_mode_music);

    // start the tune running in the background
    music_data->async_wait_ticks = mp_hal_ticks_ms();
    music_data->async_loop = args[3].u_bool;
    music_data->async_notes_len = len;
    music_data->async_notes_index = 0;
    if (len == 1) {
        // If a string was passed as a single note then we can't store a pointer
        // to args[0].u_obj, so instead store the single string directly (also
        // works if a tuple/list of one element was passed).
        music_data->async_note = items[0];
    } else {
        music_data->async_note = items;
    }
    music_data->async_state = ASYNC_MUSIC_STATE_NEXT_NOTE;

    if (args[2].u_bool) {
        // wait for tune to finish
        wait_async_music_idle();
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(bitsflow_music_play_obj, 0, bitsflow_music_play);

STATIC mp_obj_t bitsflow_music_pitch(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_frequency, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_duration, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_pin, MP_ARG_OBJ, {.u_obj = (mp_obj_t)MUSIC_OUTPUT_DEFAULT_PIN} },
        { MP_QSTR_wait, MP_ARG_BOOL, {.u_bool = true} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the parameters
    mp_uint_t frequency = args[0].u_int;
    mp_int_t duration = args[1].u_int;

    // Stop any ongoing background music
    music_data->async_state = ASYNC_MUSIC_STATE_IDLE;

    // Update pin modes
    bitsflow_pin_audio_select(args[2].u_obj, bitsflow_pin_mode_music);

    bool wait = args[3].u_bool;
    music_output_amplitude(MUSIC_OUTPUT_AMPLITUDE_ON);
    if (frequency != 0 && music_output_period_us(1000000 / frequency) == -1) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pitch"));
    }
    if (duration >= 0) {
        // use async machinery to stop the pitch after the duration
        music_data->async_wait_ticks = mp_hal_ticks_ms() + duration;
        music_data->async_loop = false;
        music_data->async_notes_len = 0;
        music_data->async_notes_index = 0;
        music_data->async_note = NULL;
        music_data->async_state = ASYNC_MUSIC_STATE_ARTICULATE;

        if (wait) {
            // wait for the pitch to finish
            wait_async_music_idle();
        }
    } else {
        // don't block here, since there's no reason to leave a pitch forever in a blocking C function
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(bitsflow_music_pitch_obj, 0, bitsflow_music_pitch);

STATIC mp_obj_t bitsflow_music_set_tempo(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ticks, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bpm, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[0].u_int != 0) {
        // set ticks
        music_data->ticks = args[0].u_int;
    }

    if (args[1].u_int != 0) {
        music_data->bpm = args[1].u_int;
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(bitsflow_music_set_tempo_obj, 0, bitsflow_music_set_tempo);

static mp_obj_t music_init(void) {
    music_data = m_new_obj(music_data_t);
    music_data->bpm = DEFAULT_BPM;
    music_data->ticks = DEFAULT_TICKS;
    music_data->last_octave = DEFAULT_OCTAVE;
    music_data->last_duration = DEFAULT_DURATION;
    music_data->async_state = ASYNC_MUSIC_STATE_IDLE;
    music_data->async_note = NULL;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(music___init___obj, music_init);

STATIC const mp_rom_map_elem_t bitsflow_music_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_music) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&music___init___obj) },

    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&bitsflow_music_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_tempo), MP_ROM_PTR(&bitsflow_music_set_tempo_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_tempo), MP_ROM_PTR(&bitsflow_music_get_tempo_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&bitsflow_music_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_pitch), MP_ROM_PTR(&bitsflow_music_pitch_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&bitsflow_music_stop_obj) },

    { MP_ROM_QSTR(MP_QSTR_DADADADUM), MP_ROM_PTR(&bitsflow_music_tune_dadadadum_obj) },
    { MP_ROM_QSTR(MP_QSTR_ENTERTAINER), MP_ROM_PTR(&bitsflow_music_tune_entertainer_obj) },
    { MP_ROM_QSTR(MP_QSTR_PRELUDE), MP_ROM_PTR(&bitsflow_music_tune_prelude_obj) },
    { MP_ROM_QSTR(MP_QSTR_ODE), MP_ROM_PTR(&bitsflow_music_tune_ode_obj) },
    { MP_ROM_QSTR(MP_QSTR_NYAN), MP_ROM_PTR(&bitsflow_music_tune_nyan_obj) },
    { MP_ROM_QSTR(MP_QSTR_RINGTONE), MP_ROM_PTR(&bitsflow_music_tune_ringtone_obj) },
    { MP_ROM_QSTR(MP_QSTR_FUNK), MP_ROM_PTR(&bitsflow_music_tune_funk_obj) },
    { MP_ROM_QSTR(MP_QSTR_BLUES), MP_ROM_PTR(&bitsflow_music_tune_blues_obj) },
    { MP_ROM_QSTR(MP_QSTR_BIRTHDAY), MP_ROM_PTR(&bitsflow_music_tune_birthday_obj) },
    { MP_ROM_QSTR(MP_QSTR_WEDDING), MP_ROM_PTR(&bitsflow_music_tune_wedding_obj) },
    { MP_ROM_QSTR(MP_QSTR_FUNERAL), MP_ROM_PTR(&bitsflow_music_tune_funeral_obj) },
    { MP_ROM_QSTR(MP_QSTR_PUNCHLINE), MP_ROM_PTR(&bitsflow_music_tune_punchline_obj) },
    { MP_ROM_QSTR(MP_QSTR_PYTHON), MP_ROM_PTR(&bitsflow_music_tune_python_obj) },
    { MP_ROM_QSTR(MP_QSTR_BADDY), MP_ROM_PTR(&bitsflow_music_tune_baddy_obj) },
    { MP_ROM_QSTR(MP_QSTR_CHASE), MP_ROM_PTR(&bitsflow_music_tune_chase_obj) },
    { MP_ROM_QSTR(MP_QSTR_BA_DING), MP_ROM_PTR(&bitsflow_music_tune_ba_ding_obj) },
    { MP_ROM_QSTR(MP_QSTR_WAWAWAWAA), MP_ROM_PTR(&bitsflow_music_tune_wawawawaa_obj) },
    { MP_ROM_QSTR(MP_QSTR_JUMP_UP), MP_ROM_PTR(&bitsflow_music_tune_jump_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_JUMP_DOWN), MP_ROM_PTR(&bitsflow_music_tune_jump_down_obj) },
    { MP_ROM_QSTR(MP_QSTR_POWER_UP), MP_ROM_PTR(&bitsflow_music_tune_power_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_POWER_DOWN), MP_ROM_PTR(&bitsflow_music_tune_power_down_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_music_locals_dict, bitsflow_music_locals_dict_table);

const mp_obj_module_t music_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&bitsflow_music_locals_dict,
};
