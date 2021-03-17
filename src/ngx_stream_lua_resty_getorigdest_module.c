#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_stream_lua_resty_getorigdest_module.h"

#include "ngx_stream_lua_socket_tcp.h"

// SO_ORIGINAL_DST
#include <linux/netfilter_ipv4.h>

static ngx_stream_module_t ngx_stream_lua_resty_getorigdest_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL                                   /* merge server configuration */
};

ngx_module_t ngx_stream_lua_resty_getorigdest_module = {
    NGX_MODULE_V1,
    &ngx_stream_lua_resty_getorigdest_module_ctx,   /* module context */
    NULL,                                           /* module directives */
    NGX_STREAM_MODULE,                              /* module type */
    NULL,                                           /* init master */
    NULL,                                           /* init module */
    NULL,                                           /* init process */
    NULL,                                           /* init thread */
    NULL,                                           /* exit thread */
    NULL,                                           /* exit process */
    NULL,                                           /* exit master */
    NGX_MODULE_V1_PADDING
};

int ngx_stream_lua_resty_getorigdest_module_getorigdest(ngx_stream_lua_socket_tcp_upstream_t *u, char* buf, int buflen)
{
	ngx_connection_t *c;
	
	if (buflen < 22) {
		return NGX_ABORT;
	}
	
	c = u->peer.connection;
	
	if (c->fd == (ngx_socket_t) -1) {
        return NGX_ERROR;
    }
	
	struct sockaddr_in orig;
	socklen_t origlen = sizeof(orig);
	
	if (getsockopt(c->fd, SOL_IP, SO_ORIGINAL_DST, (struct sockaddr *) &orig, &origlen) != 0) {
		return NGX_ERROR;
	}
	
	inet_ntop(AF_INET, (void *)&orig.sin_addr, buf, buflen);
	int portnumber = ntohs(orig.sin_port);
	ssize_t addrend = strlen(buf);
	sprintf(&buf[addrend], ":%d", portnumber);
	return NGX_OK;
}
