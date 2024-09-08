#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_stream_lua_resty_getorigdest_module.h"

#include "ngx_stream_lua_socket_tcp.h"

#ifdef __linux__
#include <linux/netfilter_ipv4.h>   // SO_ORIGINAL_DST for Linux
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/pfvar.h>             // For pf
#include <arpa/inet.h>
#endif

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
	#ifdef __linux__
    //Linux: Use SO_ORIGINAL_DST
	if (getsockopt(c->fd, SOL_IP, SO_ORIGINAL_DST, (struct sockaddr *) &orig, &origlen) != 0) {
		return NGX_ERROR;
	}
	#elif defined(__FreeBSD__)
    //FreeBSD: Use PF (Packet Filter)
    int dev = open("/dev/pf", O_RDWR);
    if (dev == -1) {
		ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Failed to open /dev/pf: %s", strerror(errno));
		return NGX_ERROR;
	}

    struct pfioc_natlook pnl;
	memset(&pnl, 0, sizeof(pnl));
	pnl.af = AF_INET;
	pnl.proto = IPPROTO_TCP;
	pnl.direction = PF_OUT;

    struct sockaddr_in source;
	struct sockaddr_in destination;
	struct sockaddr_in local;
	socklen_t addr_len = sizeof(local);
	// Get ip of interface with getsockname where request came from
	// Otherwise sockaddr will return 0.0.0.0 if openresty listen on all interfaces
	// Which lead to child process crash
	if (getsockname(c->fd, (struct sockaddr *)&local, &addr_len) == -1) {
		ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "Getsockname failed.");
		return NGX_ERROR;
	}
	source.sin_addr.s_addr = ((struct sockaddr_in *)c->sockaddr)->sin_addr.s_addr;
	source.sin_port = ((struct sockaddr_in *)c->sockaddr)->sin_port;
	memcpy(&pnl.saddr.v4, &source.sin_addr.s_addr, sizeof(pnl.saddr.v4));
	memcpy(&pnl.daddr.v4, &local.sin_addr.s_addr, sizeof(pnl.daddr.v4));
	pnl.sport = source.sin_port;
	pnl.dport = local.sin_port;

	if (ioctl(dev, DIOCNATLOOK, &pnl) == -1) {
		ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "ioctl DIOCNATLOOK failed: %s", strerror(errno));
		close(dev);
		return NGX_ERROR;
	}

	memset(&destination, 0, sizeof(struct sockaddr_in));
	destination.sin_len = sizeof(struct sockaddr_in);
	destination.sin_family = AF_INET;
	memcpy(&destination.sin_addr.s_addr, &pnl.rdaddr.v4, sizeof(destination.sin_addr.s_addr));
	destination.sin_port = pnl.rdport;

	orig.sin_addr = destination.sin_addr;
	orig.sin_port = destination.sin_port;
	close(dev);
#else
	return NGX_ERROR; //Unsupported platform
#endif
	inet_ntop(AF_INET, (void *)&orig.sin_addr, buf, buflen);
	int portnumber = ntohs(orig.sin_port);
	ssize_t addrend = strlen(buf);
	sprintf(&buf[addrend], ":%d", portnumber);
	return NGX_OK;
}
