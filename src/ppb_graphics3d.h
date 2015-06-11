/*
 * Copyright © 2013-2015  Rinat Ibragimov
 *
 * This file is part of FreshPlayerPlugin.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FPP_PPB_GRAPHICS3D_H
#define FPP_PPB_GRAPHICS3D_H

#include <ppapi/c/ppb_graphics_3d.h>


int32_t
ppb_graphics3d_get_attrib_max_value(PP_Resource instance, int32_t attribute, int32_t *value);

PP_Resource
ppb_graphics3d_create(PP_Instance instance, PP_Resource share_context, const int32_t attrib_list[]);

PP_Bool
ppb_graphics3d_is_graphics3d(PP_Resource resource);

int32_t
ppb_graphics3d_get_attribs(PP_Resource context, int32_t attrib_list[]);

int32_t
ppb_graphics3d_set_attribs(PP_Resource context, const int32_t attrib_list[]);

int32_t
ppb_graphics3d_get_error(PP_Resource context);

int32_t
ppb_graphics3d_resize_buffers(PP_Resource context, int32_t width, int32_t height);

int32_t
ppb_graphics3d_swap_buffers(PP_Resource context, struct PP_CompletionCallback callback);

#endif // FPP_PPB_GRAPHICS3D_H
