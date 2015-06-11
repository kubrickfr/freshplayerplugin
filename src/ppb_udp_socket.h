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

#ifndef FPP_PPB_UDP_SOCKET_H
#define FPP_PPB_UDP_SOCKET_H

#include <ppapi/c/private/ppb_udp_socket_private.h>


PP_Resource
ppb_udp_socket_create(PP_Instance instance_id);

PP_Bool
ppb_udp_socket_is_udp_socket(PP_Resource resource_id);

int32_t
ppb_udp_socket_set_socket_feature(PP_Resource udp_socket, PP_UDPSocketFeature_Private name,
                                  struct PP_Var value);

int32_t
ppb_udp_socket_bind(PP_Resource udp_socket, const struct PP_NetAddress_Private *addr,
                    struct PP_CompletionCallback callback);

PP_Bool
ppb_udp_socket_get_bound_address(PP_Resource udp_socket, struct PP_NetAddress_Private *addr);

int32_t
ppb_udp_socket_recv_from(PP_Resource udp_socket, char *buffer, int32_t num_bytes,
                         struct PP_CompletionCallback callback);

PP_Bool
ppb_udp_socket_get_recv_from_address(PP_Resource udp_socket,
                                     struct PP_NetAddress_Private *addr);

int32_t
ppb_udp_socket_send_to(PP_Resource udp_socket, const char *buffer, int32_t num_bytes,
                       const struct PP_NetAddress_Private *addr,
                       struct PP_CompletionCallback callback);

void
ppb_udp_socket_close(PP_Resource udp_socket);

#endif // FPP_PPB_UDP_SOCKET_H
