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

#ifndef FPP_PPB_FLASH_DRM_H
#define FPP_PPB_FLASH_DRM_H

#include <ppapi/c/private/ppb_flash_drm.h>


PP_Resource
ppb_flash_drm_create(PP_Instance instance);

int32_t
ppb_flash_drm_get_device_id(PP_Resource drm, struct PP_Var *id,
                            struct PP_CompletionCallback callback);

PP_Bool
ppb_flash_drm_get_hmonitor(PP_Resource drm, int64_t *hmonitor);

int32_t
ppb_flash_drm_get_voucher_file(PP_Resource drm, PP_Resource *file_ref,
                               struct PP_CompletionCallback callback);

int32_t
ppb_flash_drm_monitor_is_external(PP_Resource drm, PP_Bool *is_external,
                                  struct PP_CompletionCallback callback);


#endif // FPP_PPB_FLASH_DRM_H

