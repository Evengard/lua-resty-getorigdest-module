#ifndef _NGX_stream_LUA_RESTY_GETORIGDEST_MODULE_H_INCLUDED_
#define _NGX_stream_LUA_RESTY_GETORIGDEST_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_stream_lua_socket_tcp.h"

int ngx_stream_lua_resty_getorigdest_module_getorigdest(ngx_stream_lua_socket_tcp_upstream_t *u, char* buf, int buflen);

#endif // _NGX_stream_LUA_RESTY_GETORIGDEST_MODULE_H_INCLUDED_
