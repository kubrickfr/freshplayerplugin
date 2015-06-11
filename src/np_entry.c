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

#include <dlfcn.h>
#include <npapi/npapi.h>
#include <npapi/npfunctions.h>
#include <string.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/pp_errors.h>
#include <ppapi/c/pp_module.h>
#include <parson/parson.h>
#include <libgen.h>
#include "trace.h"
#include "tables.h"
#include "config.h"
#include "reverse_constant.h"
#include "pp_interface.h"
#include <glib.h>
#include "compat.h"
#include "np_entry.h"
#include "ppb_message_loop.h"
#include "tables.h"
#include "main_thread.h"
#include "ppb_core.h"


static void *module_dl_handler;
static gchar *module_version;
static gchar *module_descr;
static GList *tried_files = NULL;
static gchar *module_file_name = NULL;
static struct pp_instance_s *aux_instance = NULL;

static
void
use_fallback_version_strings(void)
{
    module_version = g_strdup(fpp_config_get_default_plugin_version());
    module_descr = g_strdup(fpp_config_get_default_plugin_descr());
}

GList *
np_entry_get_tried_plugin_files(void)
{
    return tried_files;
}

gchar *
np_entry_get_module_file_name(void)
{
    return module_file_name;
}

struct call_plugin_init_module_param_s {
    PP_Resource     m_loop;
    int             depth;
    int32_t       (*ppp_initialize_module)(PP_Module module_id,
                                           PPB_GetInterface get_browser_interface);
    int             result;
};

static
void
call_plugin_init_module_comt(void *user_data, int32_t result)
{
    struct call_plugin_init_module_param_s *p = user_data;

    // TODO: make module ids distinct
    p->result = p->ppp_initialize_module(42, ppb_get_interface);

    ppb_message_loop_post_quit_depth(p->m_loop, PP_FALSE, p->depth);
}


static
void
call_plugin_init_module_prepare_comt(void *user_data, int32_t result)
{
    ppb_core_trampoline_to_main_thread(PP_MakeCCB(call_plugin_init_module_comt, user_data), PP_OK,
                                       __func__);
}

static
uintptr_t
do_load_ppp_module(const char *fname)
{
    tried_files = g_list_prepend(tried_files, g_strdup(fname));

    module_dl_handler = dlopen(fname, RTLD_LAZY);
    if (!module_dl_handler) {
        trace_info_f("%s, can't open %s\n", __func__, fname);
        return 1;
    }

    int32_t (*ppp_initialize_module)(PP_Module module_id, PPB_GetInterface get_browser_interface);
    ppp_initialize_module = dlsym(module_dl_handler, "PPP_InitializeModule");
    ppp_get_interface = dlsym(module_dl_handler, "PPP_GetInterface");

    if (!ppp_initialize_module || !ppp_get_interface) {
        trace_error("%s, one of required PPP_* is missing\n", __func__);
        dlclose(module_dl_handler);
        module_dl_handler = NULL;
        return 1;
    }

    if (!aux_instance) {
        aux_instance = calloc(1, sizeof(*aux_instance));
        if (!aux_instance)
            return 1;

        aux_instance->id = tables_generate_new_pp_instance_id();
        tables_add_pp_instance(aux_instance->id, aux_instance);
    }

    if (ppb_message_loop_get_current() == 0) {
        // allocate message loop for browser thread
        PP_Resource message_loop = ppb_message_loop_create(aux_instance->id);
        ppb_message_loop_attach_to_current_thread(message_loop);
        ppb_message_loop_proclaim_this_thread_browser();
    }

    if (ppb_message_loop_get_for_main_thread() == 0) {
        pthread_barrier_init(&aux_instance->main_thread_barrier, NULL, 2);
        pthread_create(&aux_instance->main_thread, NULL, fresh_wrapper_main_thread, aux_instance);
        pthread_detach(aux_instance->main_thread);
        pthread_barrier_wait(&aux_instance->main_thread_barrier);
        pthread_barrier_destroy(&aux_instance->main_thread_barrier);
    }

    module_file_name = g_strdup(fname);

    if (!fpp_config_plugin_has_manifest()) {
        use_fallback_version_strings();
        return 0;
    }

    // try to read manifest.json file (only for those who can have it)
    char *manifest_dir = strdup(fname);
    gchar *manifest_path = g_strdup_printf("%s/manifest.json", dirname(manifest_dir));
    free(manifest_dir);

    JSON_Value *root_val = json_parse_file(manifest_path);
    g_free(manifest_path);
    if (!root_val) {
        use_fallback_version_strings();
        return 0;
    }

    JSON_Object *root_obj = json_value_get_object(root_val);
    const char *version = json_object_get_string(root_obj, "version");
    if (version) {
        int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
        module_version = g_strdup(version);
        (void)sscanf(module_version, "%9d.%9d.%9d.%9d", &v1, &v2, &v3, &v4);
        module_descr = g_strdup_printf("%s %d.%d r%d", fpp_config_get_plugin_name(), v1, v2, v3);
    } else {
        use_fallback_version_strings();
    }

    json_value_free(root_val);
    return 0;
}

static
uintptr_t
load_ppp_module()
{
    if (module_dl_handler) {
        // already loaded
        return 0;
    }

    fpp_config_initialize();

    if (tried_files) {
        g_list_free_full(tried_files, g_free);
        tried_files = NULL;
    }

    if (fpp_config_get_plugin_path()) {
        const char *ptr = fpp_config_get_plugin_path();
        const char *last = strchr(ptr, ':');
        uintptr_t   ret;

        // parse ':'-separated list
        while (last != NULL) {
            // try entries one by one
            char *entry = strndup(ptr, last - ptr);
            ret = do_load_ppp_module(entry);
            free(entry);
            if (ret == 0)
                return 0;

            ptr = last + 1;
            last = strchr(ptr, ':');
        }

        // and the last entry
        ret = do_load_ppp_module(ptr);
        if (ret == 0)
            return 0;

        goto failure;
    }

    // try all paths
    const char **path_list = fpp_config_get_plugin_path_list();
    while (*path_list) {
        gchar *fname = g_strdup_printf("%s/%s", *path_list, fpp_config_get_plugin_file_name());
        uintptr_t ret = do_load_ppp_module(fname);
        g_free(fname);
        if (ret == 0)
            return 0;
        path_list ++;
    }

failure:
    config.quirks.plugin_missing = 1;
    use_fallback_version_strings();
    trace_error("%s, can't find %s\n", __func__, fpp_config_get_plugin_file_name());
    return 1;
}

static
void
unload_ppp_module(void)
{
    void (*ppp_shutdown_module)(void);

    if (!module_dl_handler) {
        // module not loaded
        return;
    }

    g_free(module_descr); module_descr = NULL;
    g_free(module_version); module_version = NULL;
    g_free(module_file_name); module_file_name = NULL;
    if (tried_files) {
        g_list_free_full(tried_files, g_free);
        tried_files = NULL;
    }

    // call module shutdown handler if exists
    ppp_shutdown_module = dlsym(module_dl_handler, "PPP_ShutdownModule");
    if (ppp_shutdown_module)
        ppp_shutdown_module();

    dlclose(module_dl_handler);
    module_dl_handler = NULL;

    fpp_config_destroy();
}


__attribute__((visibility("default")))
const char *
NP_GetMIMEDescription(void)
{
    trace_info_f("[NP] %s\n", __func__);
    return fpp_config_get_plugin_mime_type();
}

__attribute__((visibility("default")))
char *
NP_GetPluginVersion(void)
{
    trace_info_f("[NP] %s\n", __func__);
    load_ppp_module();
    return module_version;
}

__attribute__((visibility("default")))
NPError
NP_GetValue(void *instance, NPPVariable variable, void *value)
{
    trace_info_f("[NP] %s instance=%p, variable=%s, value=%p\n", __func__, instance,
                 reverse_npp_variable(variable), value);
    load_ppp_module();
    switch (variable) {
    case NPPVpluginNameString:
        *(const char **)value = fpp_config_get_plugin_name();
        break;
    case NPPVpluginDescriptionString:
        *(char **)value = module_descr;
        break;
    default:
        trace_info_z("    not implemented variable %d\n", variable);
    }
    return NPERR_NO_ERROR;
}

static
int
x_error_handler(Display *dpy, XErrorEvent *ee)
{
    trace_error("[NP] caught Xlib error %d\n", ee->error_code);
    return 0;
}

static
int
x_io_error_hanlder(Display *dpy)
{
    // IO errors can't be ignored, they always terminate program.
    // Let's crash here to get core file!
    trace_error("[NP] got Xlib IO error\n");
    abort();
    return 0;
}

__attribute__((visibility("default")))
NPError
NP_Initialize(NPNetscapeFuncs *aNPNFuncs, NPPluginFuncs *aNPPFuncs)
{
    trace_info_f("[NP] %s aNPNFuncs=%p, aNPPFuncs=%p, browser API version = %u\n", __func__,
                 aNPNFuncs, aNPPFuncs, aNPNFuncs->version);

    // set logging-only error handler.
    // Ignore a previous one, we have no plans to restore it
    (void)XSetErrorHandler(x_error_handler);
    (void)XSetIOErrorHandler(x_io_error_hanlder);

    memset(&npn, 0, sizeof(npn));
    memcpy(&npn, aNPNFuncs, sizeof(npn) < aNPNFuncs->size ? sizeof(npn) : aNPNFuncs->size);

    NPPluginFuncs pf;
    memset(&pf, 0, sizeof(NPPluginFuncs));
    pf.size = MIN(aNPPFuncs->size, sizeof(NPPluginFuncs));

    // browser is supposed to fill .size and .version
    pf.newp =               NPP_New;
    pf.destroy =            NPP_Destroy;
    pf.setwindow =          NPP_SetWindow;
    pf.newstream =          NPP_NewStream;
    pf.destroystream =      NPP_DestroyStream;
    pf.asfile =             NPP_StreamAsFile;
    pf.writeready =         NPP_WriteReady;
    pf.write =              NPP_Write;
    pf.print =              NPP_Print;
    pf.event =              NPP_HandleEvent;
    pf.urlnotify =          NPP_URLNotify;
    pf.getvalue =           NPP_GetValue;
    pf.setvalue =           NPP_SetValue;
    pf.gotfocus =           NPP_GotFocus;
    pf.lostfocus =          NPP_LostFocus;
    pf.urlredirectnotify =  NPP_URLRedirectNotify;
    pf.clearsitedata =      NPP_ClearSiteData;
    pf.getsiteswithdata =   NPP_GetSitesWithData;
    pf.didComposite =       NPP_DidComposite;

    memcpy(aNPPFuncs, &pf, pf.size);

    if (tables_open_display() != 0)
        return NPERR_GENERIC_ERROR;

    if (aNPNFuncs->version < NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL) {
        config.quirks.plugin_missing = 1;
        config.quirks.incompatible_npapi_version = 1;
    }

    load_ppp_module();

    struct call_plugin_init_module_param_s *p = g_slice_alloc(sizeof(*p));
    p->m_loop = ppb_message_loop_get_for_browser_thread();
    p->depth =  ppb_message_loop_get_depth(p->m_loop) + 1;
    p->ppp_initialize_module = dlsym(module_dl_handler, "PPP_InitializeModule");

    ppb_message_loop_post_work_with_result(p->m_loop,
                                           PP_MakeCCB(call_plugin_init_module_prepare_comt, p), 0,
                                           PP_OK, p->depth, __func__);
    ppb_message_loop_run_nested(p->m_loop);
    int res = p->result;
    g_slice_free1(sizeof(*p), p);

    if (res != 0) {
        trace_error("%s, PPP_InitializeModule returned %d\n", __func__, res);
        return NPERR_GENERIC_ERROR;
    }

    return NPERR_NO_ERROR;
}

__attribute__((visibility("default")))
NPError
NP_Shutdown(void)
{
    trace_info_f("[NP] %s\n", __func__);

    unload_ppp_module();
    tables_close_display();

    return NPERR_NO_ERROR;
}
