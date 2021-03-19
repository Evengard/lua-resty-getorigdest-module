local ffi = require "ffi"
local C = ffi.C

local SOCKET_CTX_INDEX = 1
local NGX_OK = ngx.OK

ffi.cdef [[
    typedef struct ngx_stream_lua_socket_tcp_upstream_s ngx_stream_lua_socket_tcp_upstream_t;
	int ngx_stream_lua_resty_getorigdest_module_getorigdest(ngx_stream_lua_socket_tcp_upstream_t *u, char* buf, int buflen);
]]

local getorigdestfn = function(sock)
	local u = sock[SOCKET_CTX_INDEX];
	local buflen = 25;
	local buf = ffi.new("char[?]", 25);
	local ret = C.ngx_stream_lua_resty_getorigdest_module_getorigdest(u, buf, buflen);
	if ret ~= NGX_OK then
		return nil, "getorigdest failed";
	end
	return ffi.string(buf);
end

return {
	getorigdest = getorigdestfn,
}
