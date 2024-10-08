/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Mark Shannon
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

STATIC void bitsflow_image_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t*)self_in;
    mp_printf(print, "Image(");
    if (kind == PRINT_STR) {
        mp_printf(print, "\n    ");
    }
    mp_printf(print, "'");
    for (int y = 0; y < image_height(self); ++y) {
        for (int x = 0; x < image_width(self); ++x) {
            mp_printf(print, "%c", "0123456789"[image_get_pixel(self, x, y)]);
        }
        mp_printf(print, ":");
        if (kind == PRINT_STR && y < image_height(self)-1)
            mp_printf(print, "'\n    '");
    }
    mp_printf(print, "'");
    if (kind == PRINT_STR) {
        mp_printf(print, "\n");
    }
    mp_printf(print, ")");
}

STATIC bitsflow_image_obj_t *image_from_parsed_str(const char *s, mp_int_t len) {
    mp_int_t w = 0;
    mp_int_t h = 0;
    mp_int_t line_len = 0;
    greyscale_t *result;
    // First pass -- Establish metadata
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n' || c == ':') {
            w = MAX(line_len, w);
            line_len = 0;
            ++h;
        } else if (c == ' ') {
            ++line_len;
        } else if ('c' >= '0' && c <= '9') {
            ++line_len;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("unexpected character in Image definition"));
        }
    }
    if (line_len) {
        // Omitted trailing terminator
        ++h;
        w = MAX(line_len, w);
    }
    result = greyscale_new(w, h);
    mp_int_t x = 0;
    mp_int_t y = 0;
    // Second pass -- Fill in data
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n' || c == ':') {
            while (x < w) {
                greyscale_set_pixel(result, x, y, 0);
                x++;
            }
            ++y;
            x = 0;
        } else if (c == ' ') {
            // Treat spaces as 0
            greyscale_set_pixel(result, x, y, 0);
            ++x;
        } else if ('c' >= '0' && c <= '9') {
            greyscale_set_pixel(result, x, y, c - '0');
            ++x;
        }
    }
    if (y < h) {
        while (x < w) {
            greyscale_set_pixel(result, x, y, 0);
            x++;
        }
    }
    return (bitsflow_image_obj_t *)result;
}


STATIC mp_obj_t bitsflow_image_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 0, 3, false);

    switch (n_args) {
        case 0: {
            greyscale_t *image = greyscale_new(BITSFLOW_DISPLAY_WIDTH, BITSFLOW_DISPLAY_HEIGHT);
            greyscale_clear(image);
            return image;
        }

        case 1: {
            if (MP_OBJ_IS_STR(args[0])) {
                // arg is a string object
                mp_uint_t len;
                const char *str = mp_obj_str_get_data(args[0], &len);
                // make image from string
                if (len == 1) {
                    // For a single charater, return the font glyph
                    return bitsflow_image_for_char(str[0]);
                } else {
                    // Otherwise parse the image description string
                    return image_from_parsed_str(str, len);
                }
            } else {
                mp_raise_TypeError(MP_ERROR_TEXT("Image(s) takes a string"));
            }
        }

        case 2:
        case 3: {
            mp_int_t w = mp_obj_get_int(args[0]);
            mp_int_t h = mp_obj_get_int(args[1]);
            greyscale_t *image = greyscale_new(w, h);
            if (n_args == 2) {
                greyscale_clear(image);
            } else {
                mp_buffer_info_t bufinfo;
                mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);

                if (w < 0 || h < 0 || (size_t)(w * h) != bufinfo.len) {
                    mp_raise_ValueError(MP_ERROR_TEXT("image data is incorrect size"));
                }
                mp_int_t i = 0;
                for (mp_int_t y = 0; y < h; y++) {
                    for (mp_int_t x = 0; x < w; ++x) {
                        uint8_t val = MIN(((const uint8_t*)bufinfo.buf)[i], BITSFLOW_DISPLAY_MAX_BRIGHTNESS);
                        greyscale_set_pixel(image, x, y, val);
                        ++i;
                    }
                }
            }
            return image;
        }

        default: {
            mp_raise_TypeError(MP_ERROR_TEXT("Image() takes 0 to 3 arguments"));
        }
    }
}

greyscale_t *image_shift(bitsflow_image_obj_t *self, mp_int_t x, mp_int_t y) {
    int w = image_width(self);
    int h = image_height(self);
    greyscale_t *result = greyscale_new(w, h);
    image_blit(self, result, x, y, w, h, 0, 0);
    return result;
}

STATIC bitsflow_image_obj_t *image_crop(bitsflow_image_obj_t *img, mp_int_t x, mp_int_t y, mp_int_t w, mp_int_t h) {
    if (w < 0) {
        w = 0;
    }
    if (h < 0) {
        h = 0;
    }
    greyscale_t *result = greyscale_new(w, h);
    image_blit(img, result, x, y, w, h, 0, 0);
    return (bitsflow_image_obj_t *)result;
}

mp_obj_t bitsflow_image_width(mp_obj_t self_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t*)self_in;
    return MP_OBJ_NEW_SMALL_INT(image_width(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_image_width_obj, bitsflow_image_width);

mp_obj_t bitsflow_image_height(mp_obj_t self_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t*)self_in;
    return MP_OBJ_NEW_SMALL_INT(image_height(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_image_height_obj, bitsflow_image_height);

mp_obj_t bitsflow_image_get_pixel(mp_obj_t self_in, mp_obj_t x_in, mp_obj_t y_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    mp_int_t x = mp_obj_get_int(x_in);
    mp_int_t y = mp_obj_get_int(y_in);
    if (x < 0 || y < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("index cannot be negative"));
    }
    if (x < image_width(self) && y < image_height(self)) {
        return MP_OBJ_NEW_SMALL_INT(image_get_pixel(self, x, y));
    }
    mp_raise_ValueError(MP_ERROR_TEXT("index too large"));
}
MP_DEFINE_CONST_FUN_OBJ_3(bitsflow_image_get_pixel_obj, bitsflow_image_get_pixel);

/* Raise an exception if not mutable */
static void check_mutability(bitsflow_image_obj_t *self) {
    if (self->base.five) {
        mp_raise_TypeError(MP_ERROR_TEXT("image cannot be modified (try copying first)"));
    }
}

mp_obj_t bitsflow_image_set_pixel(mp_uint_t n_args, const mp_obj_t *args) {
    (void)n_args;
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)args[0];
    check_mutability(self);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    if (x < 0 || y < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("index cannot be negative"));
    }
    mp_int_t bright = mp_obj_get_int(args[3]);
    if (bright < 0 || bright > BITSFLOW_DISPLAY_MAX_BRIGHTNESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("brightness out of bounds"));
    }
    if (x < image_width(self) && y < image_height(self)) {
        greyscale_set_pixel(&self->greyscale, x, y, bright);
        return mp_const_none;
    }
    mp_raise_ValueError(MP_ERROR_TEXT("index too large"));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(bitsflow_image_set_pixel_obj, 4, 4, bitsflow_image_set_pixel);

mp_obj_t bitsflow_image_fill(mp_obj_t self_in, mp_obj_t n_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    check_mutability(self);
    mp_int_t n = mp_obj_get_int(n_in);
    if (n < 0 || n > BITSFLOW_DISPLAY_MAX_BRIGHTNESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("brightness out of bounds"));
    }
    greyscale_fill(&self->greyscale, n);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_image_fill_obj, bitsflow_image_fill);

mp_obj_t bitsflow_image_blit(mp_uint_t n_args, const mp_obj_t *args) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)args[0];
    check_mutability(self);

    mp_obj_t src = args[1];
    if (mp_obj_get_type(src) != &bitsflow_image_type) {
        mp_raise_TypeError(MP_ERROR_TEXT("expecting an image"));
    }
    if (n_args == 7) {
        mp_raise_TypeError(MP_ERROR_TEXT("must specify both offsets"));
    }
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t w = mp_obj_get_int(args[4]);
    mp_int_t h = mp_obj_get_int(args[5]);
    if (w < 0 || h < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("size cannot be negative"));
    }
    mp_int_t xdest;
    mp_int_t ydest;
    if (n_args == 6) {
        xdest = 0;
        ydest = 0;
    } else {
        xdest = mp_obj_get_int(args[6]);
        ydest = mp_obj_get_int(args[7]);
    }
    image_blit((bitsflow_image_obj_t *)src, &(self->greyscale), x, y, w, h, xdest, ydest);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(bitsflow_image_blit_obj, 6, 8, bitsflow_image_blit);

mp_obj_t bitsflow_image_crop(mp_uint_t n_args, const mp_obj_t *args) {
    (void)n_args;
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)args[0];
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t y0 = mp_obj_get_int(args[2]);
    mp_int_t x1 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);
    return image_crop(self, x0, y0, x1, y1);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(bitsflow_image_crop_obj, 5, 5, bitsflow_image_crop);

mp_obj_t bitsflow_image_shift_left(mp_obj_t self_in, mp_obj_t n_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return image_shift(self, n, 0);
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_image_shift_left_obj, bitsflow_image_shift_left);

mp_obj_t bitsflow_image_shift_right(mp_obj_t self_in, mp_obj_t n_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return image_shift(self, -n, 0);
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_image_shift_right_obj, bitsflow_image_shift_right);

mp_obj_t bitsflow_image_shift_up(mp_obj_t self_in, mp_obj_t n_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return image_shift(self, 0, n);
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_image_shift_up_obj, bitsflow_image_shift_up);

mp_obj_t bitsflow_image_shift_down(mp_obj_t self_in, mp_obj_t n_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return image_shift(self, 0, -n);
}
MP_DEFINE_CONST_FUN_OBJ_2(bitsflow_image_shift_down_obj, bitsflow_image_shift_down);

mp_obj_t bitsflow_image_copy(mp_obj_t self_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    return image_copy(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_image_copy_obj, bitsflow_image_copy);

mp_obj_t bitsflow_image_invert(mp_obj_t self_in) {
    bitsflow_image_obj_t *self = (bitsflow_image_obj_t *)self_in;
    return image_invert(self);
}
MP_DEFINE_CONST_FUN_OBJ_1(bitsflow_image_invert_obj, bitsflow_image_invert);

STATIC const mp_rom_map_elem_t bitsflow_image_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&bitsflow_image_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&bitsflow_image_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pixel), MP_ROM_PTR(&bitsflow_image_get_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pixel), MP_ROM_PTR(&bitsflow_image_set_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_shift_left), MP_ROM_PTR(&bitsflow_image_shift_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_shift_right), MP_ROM_PTR(&bitsflow_image_shift_right_obj) },
    { MP_ROM_QSTR(MP_QSTR_shift_up), MP_ROM_PTR(&bitsflow_image_shift_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_shift_down), MP_ROM_PTR(&bitsflow_image_shift_down_obj) },
    { MP_ROM_QSTR(MP_QSTR_copy), MP_ROM_PTR(&bitsflow_image_copy_obj) },
    { MP_ROM_QSTR(MP_QSTR_crop), MP_ROM_PTR(&bitsflow_image_crop_obj) },
    { MP_ROM_QSTR(MP_QSTR_invert), MP_ROM_PTR(&bitsflow_image_invert_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&bitsflow_image_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&bitsflow_image_blit_obj) },

    { MP_ROM_QSTR(MP_QSTR_HEART), MP_ROM_PTR(&bitsflow_const_image_heart_obj) },
    { MP_ROM_QSTR(MP_QSTR_HEART_SMALL), MP_ROM_PTR(&bitsflow_const_image_heart_small_obj) },
    { MP_ROM_QSTR(MP_QSTR_HAPPY), MP_ROM_PTR(&bitsflow_const_image_happy_obj) },
    { MP_ROM_QSTR(MP_QSTR_SMILE), MP_ROM_PTR(&bitsflow_const_image_smile_obj) },
    { MP_ROM_QSTR(MP_QSTR_SAD), MP_ROM_PTR(&bitsflow_const_image_sad_obj) },
    { MP_ROM_QSTR(MP_QSTR_CONFUSED), MP_ROM_PTR(&bitsflow_const_image_confused_obj) },
    { MP_ROM_QSTR(MP_QSTR_ANGRY), MP_ROM_PTR(&bitsflow_const_image_angry_obj) },
    { MP_ROM_QSTR(MP_QSTR_ASLEEP), MP_ROM_PTR(&bitsflow_const_image_asleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_SURPRISED), MP_ROM_PTR(&bitsflow_const_image_surprised_obj) },
    { MP_ROM_QSTR(MP_QSTR_SILLY), MP_ROM_PTR(&bitsflow_const_image_silly_obj) },
    { MP_ROM_QSTR(MP_QSTR_FABULOUS), MP_ROM_PTR(&bitsflow_const_image_fabulous_obj) },
    { MP_ROM_QSTR(MP_QSTR_MEH), MP_ROM_PTR(&bitsflow_const_image_meh_obj) },
    { MP_ROM_QSTR(MP_QSTR_YES), MP_ROM_PTR(&bitsflow_const_image_yes_obj) },
    { MP_ROM_QSTR(MP_QSTR_NO), MP_ROM_PTR(&bitsflow_const_image_no_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK12), MP_ROM_PTR(&bitsflow_const_image_clock12_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK1), MP_ROM_PTR(&bitsflow_const_image_clock1_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK2), MP_ROM_PTR(&bitsflow_const_image_clock2_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK3), MP_ROM_PTR(&bitsflow_const_image_clock3_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK4), MP_ROM_PTR(&bitsflow_const_image_clock4_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK5), MP_ROM_PTR(&bitsflow_const_image_clock5_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK6), MP_ROM_PTR(&bitsflow_const_image_clock6_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK7), MP_ROM_PTR(&bitsflow_const_image_clock7_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK8), MP_ROM_PTR(&bitsflow_const_image_clock8_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK9), MP_ROM_PTR(&bitsflow_const_image_clock9_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK10), MP_ROM_PTR(&bitsflow_const_image_clock10_obj) },
    { MP_ROM_QSTR(MP_QSTR_CLOCK11), MP_ROM_PTR(&bitsflow_const_image_clock11_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_N), MP_ROM_PTR(&bitsflow_const_image_arrow_n_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_NE), MP_ROM_PTR(&bitsflow_const_image_arrow_ne_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_E), MP_ROM_PTR(&bitsflow_const_image_arrow_e_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_SE), MP_ROM_PTR(&bitsflow_const_image_arrow_se_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_S), MP_ROM_PTR(&bitsflow_const_image_arrow_s_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_SW), MP_ROM_PTR(&bitsflow_const_image_arrow_sw_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_W), MP_ROM_PTR(&bitsflow_const_image_arrow_w_obj) },
    { MP_ROM_QSTR(MP_QSTR_ARROW_NW), MP_ROM_PTR(&bitsflow_const_image_arrow_nw_obj) },
    { MP_ROM_QSTR(MP_QSTR_TRIANGLE), MP_ROM_PTR(&bitsflow_const_image_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_TRIANGLE_LEFT), MP_ROM_PTR(&bitsflow_const_image_triangle_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_CHESSBOARD), MP_ROM_PTR(&bitsflow_const_image_chessboard_obj) },
    { MP_ROM_QSTR(MP_QSTR_DIAMOND), MP_ROM_PTR(&bitsflow_const_image_diamond_obj) },
    { MP_ROM_QSTR(MP_QSTR_DIAMOND_SMALL), MP_ROM_PTR(&bitsflow_const_image_diamond_small_obj) },
    { MP_ROM_QSTR(MP_QSTR_SQUARE), MP_ROM_PTR(&bitsflow_const_image_square_obj) },
    { MP_ROM_QSTR(MP_QSTR_SQUARE_SMALL), MP_ROM_PTR(&bitsflow_const_image_square_small_obj) },
    { MP_ROM_QSTR(MP_QSTR_RABBIT), MP_ROM_PTR(&bitsflow_const_image_rabbit_obj) },
    { MP_ROM_QSTR(MP_QSTR_COW), MP_ROM_PTR(&bitsflow_const_image_cow_obj) },
    { MP_ROM_QSTR(MP_QSTR_MUSIC_CROTCHET), MP_ROM_PTR(&bitsflow_const_image_music_crotchet_obj) },
    { MP_ROM_QSTR(MP_QSTR_MUSIC_QUAVER), MP_ROM_PTR(&bitsflow_const_image_music_quaver_obj) },
    { MP_ROM_QSTR(MP_QSTR_MUSIC_QUAVERS), MP_ROM_PTR(&bitsflow_const_image_music_quavers_obj) },
    { MP_ROM_QSTR(MP_QSTR_PITCHFORK), MP_ROM_PTR(&bitsflow_const_image_pitchfork_obj) },
    { MP_ROM_QSTR(MP_QSTR_XMAS), MP_ROM_PTR(&bitsflow_const_image_xmas_obj) },
    { MP_ROM_QSTR(MP_QSTR_PACMAN), MP_ROM_PTR(&bitsflow_const_image_pacman_obj) },
    { MP_ROM_QSTR(MP_QSTR_TARGET), MP_ROM_PTR(&bitsflow_const_image_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_ALL_CLOCKS), MP_ROM_PTR(&bitsflow_const_image_all_clocks_tuple_obj) },
    { MP_ROM_QSTR(MP_QSTR_ALL_ARROWS), MP_ROM_PTR(&bitsflow_const_image_all_arrows_tuple_obj) },
    { MP_ROM_QSTR(MP_QSTR_TSHIRT), MP_ROM_PTR(&bitsflow_const_image_tshirt_obj) },
    { MP_ROM_QSTR(MP_QSTR_ROLLERSKATE), MP_ROM_PTR(&bitsflow_const_image_rollerskate_obj) },
    { MP_ROM_QSTR(MP_QSTR_DUCK), MP_ROM_PTR(&bitsflow_const_image_duck_obj) },
    { MP_ROM_QSTR(MP_QSTR_HOUSE), MP_ROM_PTR(&bitsflow_const_image_house_obj) },
    { MP_ROM_QSTR(MP_QSTR_TORTOISE), MP_ROM_PTR(&bitsflow_const_image_tortoise_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUTTERFLY), MP_ROM_PTR(&bitsflow_const_image_butterfly_obj) },
    { MP_ROM_QSTR(MP_QSTR_STICKFIGURE), MP_ROM_PTR(&bitsflow_const_image_stickfigure_obj) },
    { MP_ROM_QSTR(MP_QSTR_GHOST), MP_ROM_PTR(&bitsflow_const_image_ghost_obj) },
    { MP_ROM_QSTR(MP_QSTR_SWORD), MP_ROM_PTR(&bitsflow_const_image_sword_obj) },
    { MP_ROM_QSTR(MP_QSTR_GIRAFFE), MP_ROM_PTR(&bitsflow_const_image_giraffe_obj) },
    { MP_ROM_QSTR(MP_QSTR_SKULL), MP_ROM_PTR(&bitsflow_const_image_skull_obj) },
    { MP_ROM_QSTR(MP_QSTR_UMBRELLA), MP_ROM_PTR(&bitsflow_const_image_umbrella_obj) },
    { MP_ROM_QSTR(MP_QSTR_SNAKE), MP_ROM_PTR(&bitsflow_const_image_snake_obj) },
    { MP_ROM_QSTR(MP_QSTR_SCISSORS), MP_ROM_PTR(&bitsflow_const_image_scissors_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bitsflow_image_locals_dict, bitsflow_image_locals_dict_table);

STATIC const unsigned char *get_font_data_from_char(char c) {
    const uint8_t *data = bitsflow_hal_get_font_data(c);
    if (data == NULL) {
        data = bitsflow_hal_get_font_data('?');
    }
    return data;
}

STATIC mp_int_t get_pixel_from_font_data(const unsigned char *data, int x, int y) {
    // The following logic belongs in BitsflowFont
    return ((data[y] >> (4 - x)) & 1);
}

STATIC void bitsflow_image_set_from_char(greyscale_t *img, char c) {
    const unsigned char *data = get_font_data_from_char(c);
    for (int x = 0; x < BITSFLOW_DISPLAY_WIDTH; ++x) {
        for (int y = 0; y < BITSFLOW_DISPLAY_HEIGHT; ++y) {
            greyscale_set_pixel(img, x, y, get_pixel_from_font_data(data, x, y)*BITSFLOW_DISPLAY_MAX_BRIGHTNESS);
        }
    }
}

bitsflow_image_obj_t *bitsflow_image_for_char(char c) {
    greyscale_t *result = greyscale_new(BITSFLOW_DISPLAY_WIDTH, BITSFLOW_DISPLAY_HEIGHT);
    bitsflow_image_set_from_char(result, c);
    return (bitsflow_image_obj_t *)result;
}

bitsflow_image_obj_t *bitsflow_image_dim(bitsflow_image_obj_t *lhs, mp_float_t fval) {
    if (fval < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("brightness multiplier must not be negative"));
    }
    greyscale_t *result = greyscale_new(image_width(lhs), image_height(lhs));
    for (int x = 0; x < image_width(lhs); ++x) {
        for (int y = 0; y < image_width(lhs); ++y) {
            int val = MIN((int)image_get_pixel(lhs, x, y) * fval + 0.5, BITSFLOW_DISPLAY_MAX_BRIGHTNESS);
            greyscale_set_pixel(result, x, y, val);
        }
    }
    return (bitsflow_image_obj_t *)result;
}

STATIC bitsflow_image_obj_t *bitsflow_image_sum(bitsflow_image_obj_t *lhs, bitsflow_image_obj_t *rhs, bool add) {
    mp_int_t h = image_height(lhs);
    mp_int_t w = image_width(lhs);
    if (image_height(rhs) != h || image_width(rhs) != w) {
        mp_raise_ValueError(MP_ERROR_TEXT("images must be the same size"));
    }
    greyscale_t *result = greyscale_new(w, h);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int val;
            int lval = image_get_pixel(lhs, x, y);
            int rval = image_get_pixel(rhs, x, y);
            if (add) {
                val = MIN(lval + rval, BITSFLOW_DISPLAY_MAX_BRIGHTNESS);
            } else {
                val = MAX(0, lval - rval);
            }
            greyscale_set_pixel(result, x, y, val);
        }
    }
    return (bitsflow_image_obj_t *)result;
}

STATIC mp_obj_t image_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    if (mp_obj_get_type(lhs_in) != &bitsflow_image_type) {
        return MP_OBJ_NULL; // op not supported
    }
    bitsflow_image_obj_t *lhs = (bitsflow_image_obj_t *)lhs_in;
    switch(op) {
        case MP_BINARY_OP_ADD:
        case MP_BINARY_OP_SUBTRACT:
            break;
        case MP_BINARY_OP_MULTIPLY:
            return bitsflow_image_dim(lhs, mp_obj_get_float(rhs_in));
        case MP_BINARY_OP_TRUE_DIVIDE:
            return bitsflow_image_dim(lhs, 1.0/mp_obj_get_float(rhs_in));
        default:
            return MP_OBJ_NULL; // op not supported
    }
    if (mp_obj_get_type(rhs_in) != &bitsflow_image_type) {
        return MP_OBJ_NULL; // op not supported
    }
    return bitsflow_image_sum(lhs, (bitsflow_image_obj_t *)rhs_in, op == MP_BINARY_OP_ADD);
}


const mp_obj_type_t bitsflow_image_type = {
    { &mp_type_type },
    .name = MP_QSTR_BitsflowImage,
    .print = bitsflow_image_print,
    .make_new = bitsflow_image_make_new,
    .binary_op = image_binary_op,
    .locals_dict = (mp_obj_dict_t*)&bitsflow_image_locals_dict,
};

///////////////////////////////////////////////////////////////////////////////////

typedef struct _scrolling_string_t {
    mp_obj_base_t base;
    char const *str;
    mp_uint_t len;
    mp_obj_t ref;
    bool monospace;
    bool repeat;
} scrolling_string_t;

typedef struct _scrolling_string_iterator_t {
    mp_obj_base_t base;
    mp_obj_t ref;
    greyscale_t *img;
    char const *next_char;
    char const *start;
    char const *end;
    uint8_t offset;
    uint8_t offset_limit;
    bool monospace;
    bool repeat;
    char right;
} scrolling_string_iterator_t;

extern const mp_obj_type_t bitsflow_scrolling_string_type;
extern const mp_obj_type_t bitsflow_scrolling_string_iterator_type;

mp_obj_t scrolling_string_image_iterable(const char* str, mp_uint_t len, mp_obj_t ref, bool monospace, bool repeat) {
    scrolling_string_t *result = m_new_obj(scrolling_string_t);
    result->base.type = &bitsflow_scrolling_string_type;
    result->str = str;
    result->len = len;
    result->ref = ref;
    result->monospace = monospace;
    result->repeat = repeat;
    return result;
}

STATIC int font_column_non_blank(const unsigned char *font_data, unsigned int col) {
    for (int y = 0; y < BITSFLOW_DISPLAY_HEIGHT; ++y) {
        if (get_pixel_from_font_data(font_data, col, y)) {
            return 1;
        }
    }
    return 0;
}

/* Not strictly the rightmost non-blank column, but the rightmost in columns 2,3 or 4. */
STATIC unsigned int rightmost_non_blank_column(const unsigned char *font_data) {
    if (font_column_non_blank(font_data, 4)) {
        return 4;
    }
    if (font_column_non_blank(font_data, 3)) {
        return 3;
    }
    return 2;
}

static void restart(scrolling_string_iterator_t *iter) {
    iter->next_char = iter->start;
    iter->offset = 0;
    if (iter->start < iter->end) {
        iter->right = *iter->next_char;
        if (iter->monospace) {
            iter->offset_limit = 5;
        } else {
            iter->offset_limit = rightmost_non_blank_column(get_font_data_from_char(iter->right)) + 1;
        }
    } else {
        iter->right = ' ';
        iter->offset_limit = 5;
    }
}

STATIC mp_obj_t get_bitsflow_scrolling_string_iter(mp_obj_t o_in, mp_obj_iter_buf_t *iter_buf) {
    (void)iter_buf; // not big enough to hold scrolling_string_iterator_t
    scrolling_string_t *str = (scrolling_string_t *)o_in;
    scrolling_string_iterator_t *result = m_new_obj(scrolling_string_iterator_t);
    result->base.type = &bitsflow_scrolling_string_iterator_type;
    result->img = greyscale_new(BITSFLOW_DISPLAY_WIDTH, BITSFLOW_DISPLAY_HEIGHT);
    result->start = str->str;
    result->ref = str->ref;
    result->monospace = str->monospace;
    result->end = result->start + str->len;
    result->repeat = str->repeat;
    restart(result);
    return result;
}

STATIC mp_obj_t bitsflow_scrolling_string_iter_next(mp_obj_t o_in) {
    scrolling_string_iterator_t *iter = (scrolling_string_iterator_t *)o_in;
    if (iter->next_char == iter->end && iter->offset == 5) {
        if (iter->repeat) {
            restart(iter);
            greyscale_clear(iter->img);
        } else {
            return MP_OBJ_STOP_ITERATION;
        }
    }
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < BITSFLOW_DISPLAY_HEIGHT; y++) {
            greyscale_set_pixel(iter->img, x, y, greyscale_get_pixel(iter->img, x+1, y));
        }
    }
    for (int y = 0; y < BITSFLOW_DISPLAY_HEIGHT; y++) {
        greyscale_set_pixel(iter->img, 4, y, 0);
    }
    const unsigned char *font_data;
    if (iter->offset < iter->offset_limit) {
        font_data = get_font_data_from_char(iter->right);
        for (int y = 0; y < BITSFLOW_DISPLAY_HEIGHT; ++y) {
            int pix = get_pixel_from_font_data(font_data, iter->offset, y)*BITSFLOW_DISPLAY_MAX_BRIGHTNESS;
            greyscale_set_pixel(iter->img, 4, y, pix);
        }
    } else if (iter->offset == iter->offset_limit) {
        ++iter->next_char;
        if (iter->next_char == iter->end) {
            iter->right = ' ';
            iter->offset_limit = 5;
            iter->offset = 0;
        } else {
            iter->right = *iter->next_char;
            font_data = get_font_data_from_char(iter->right);
            if (iter->monospace) {
                iter->offset = -1;
                iter->offset_limit = 5;
            } else {
                iter->offset = -font_column_non_blank(font_data, 0);
                iter->offset_limit = rightmost_non_blank_column(font_data)+1;
            }
        }
    }
    ++iter->offset;
    return iter->img;
}

const mp_obj_type_t bitsflow_scrolling_string_type = {
    { &mp_type_type },
    .name = MP_QSTR_ScrollingString,
    .getiter = get_bitsflow_scrolling_string_iter,
};

const mp_obj_type_t bitsflow_scrolling_string_iterator_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .getiter = mp_identity_getiter,
    .iternext = bitsflow_scrolling_string_iter_next,
};

///////////////////////////////////////////////////////////////////////////////////

/** Facade types to present a string as a sequence of images.
 * These are necessary to avoid allocation during iteration,
 * which may happen in interrupt handlers.
 */

typedef struct _string_image_facade_t {
    mp_obj_base_t base;
    mp_obj_t string;
    greyscale_t *image;
} string_image_facade_t;

static mp_obj_t string_image_facade_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value) {
    if (value == MP_OBJ_SENTINEL) {
        // Fill in image
        string_image_facade_t *self = (string_image_facade_t *)self_in;
        mp_uint_t len;
        const char *text = mp_obj_str_get_data(self->string, &len);
        mp_uint_t index = mp_get_index(self->base.type, len, index_in, false);
        bitsflow_image_set_from_char(self->image, text[index]);
        return self->image;
    } else {
        return MP_OBJ_NULL; // op not supported
    }
}

static mp_obj_t facade_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    string_image_facade_t *self = (string_image_facade_t *)self_in;
    switch (op) {
        case MP_UNARY_OP_LEN:
            return mp_obj_len(self->string);
        default:
            return MP_OBJ_NULL; // op not supported
    }
}

static mp_obj_t bitsflow_facade_iterator(mp_obj_t iterable_in, mp_obj_iter_buf_t *iter_buf);

const mp_obj_type_t string_image_facade_type = {
    { &mp_type_type },
    .name = MP_QSTR_Facade,
    .unary_op = facade_unary_op,
    .subscr = string_image_facade_subscr,
    .getiter = bitsflow_facade_iterator,
};

///////////////////////////////////////////////////////////////////////////////////

typedef struct _facade_iterator_t {
    mp_obj_base_t base;
    mp_obj_t string;
    mp_uint_t index;
    greyscale_t *image;
} facade_iterator_t;

mp_obj_t bitsflow_string_facade(mp_obj_t string) {
    string_image_facade_t *result = m_new_obj(string_image_facade_t);
    result->base.type = &string_image_facade_type;
    result->string = string;
    result->image = greyscale_new(BITSFLOW_DISPLAY_WIDTH, BITSFLOW_DISPLAY_HEIGHT);
    return result;
}

static mp_obj_t bitsflow_facade_iter_next(mp_obj_t iter_in) {
    facade_iterator_t *iter = (facade_iterator_t *)iter_in;
    mp_uint_t len;
    const char *text = mp_obj_str_get_data(iter->string, &len);
    if (iter->index >= len) {
        return MP_OBJ_STOP_ITERATION;
    }
    bitsflow_image_set_from_char(iter->image, text[iter->index]);
    iter->index++;
    return iter->image;
}

const mp_obj_type_t bitsflow_facade_iterator_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .getiter = mp_identity_getiter,
    .iternext = bitsflow_facade_iter_next,
};

static mp_obj_t bitsflow_facade_iterator(mp_obj_t iterable_in, mp_obj_iter_buf_t *iter_buf) {
    assert(sizeof(facade_iterator_t) <= sizeof(mp_obj_iter_buf_t));
    facade_iterator_t *result = (facade_iterator_t*)iter_buf;
    string_image_facade_t *iterable = (string_image_facade_t *)iterable_in;
    result->base.type = &bitsflow_facade_iterator_type;
    result->string = iterable->string;
    result->image = iterable->image;
    result->index = 0;
    return result;
}
