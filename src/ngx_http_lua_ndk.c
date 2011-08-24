#ifndef DDEBUG
#define DDEBUG 0
#endif
#include "ddebug.h"


#include "ngx_http_lua_ndk.h"


#if defined(NDK) && NDK


static ndk_set_var_value_pt ngx_http_lookup_ndk_set_var_directive(u_char *name,
        size_t name_len);


int
ngx_http_lua_ndk_set_var_get(lua_State *L)
{
    ndk_set_var_value_pt                 func;
    size_t                               len;
    u_char                              *p;

    p = (u_char *) luaL_checklstring(L, 2, &len);

    dd("ndk.set_var metatable __index: %s", p);

    func = ngx_http_lookup_ndk_set_var_directive(p, len);

    if (func == NULL) {
        return luaL_error(L, "ndk.set_var: directive \"%s\" not found "
                "or does not use ndk_set_var_value",
                p);

    }

    lua_pushvalue(L, -1); /* table key key */
    lua_pushvalue(L, -1); /* table key key key */

    lua_pushlightuserdata(L, func); /* table key key key func */

    lua_pushcclosure(L, ngx_http_lua_run_set_var_directive, 2);
        /* table key key closure */

    lua_rawset(L, 1); /* table key */

    lua_rawget(L, 1); /* table closure */

    return 1;
}


int
ngx_http_lua_ndk_set_var_set(lua_State *L)
{
    return luaL_error(L, "Not allowed");
}


int
ngx_http_lua_run_set_var_directive(lua_State *L)
{
    ngx_int_t                            rc;
    ndk_set_var_value_pt                 func;
    ngx_str_t                            res;
    ngx_http_variable_value_t            arg;
    u_char                              *p;
    size_t                               len;
    ngx_http_request_t                  *r;

    if (lua_gettop(L) != 1) {
        return luaL_error(L, "expecting one argument");
    }

    arg.data = (u_char *) luaL_checklstring(L, 1, &len);
    arg.len = len;

    lua_getglobal(L, GLOBALS_SYMBOL_REQUEST);
    r = lua_touserdata(L, -1);
    lua_pop(L, 1);

    if (r == NULL) {
        return luaL_error(L, "no request object found");
    }

    p = (u_char *) luaL_checklstring(L, lua_upvalueindex(1), &len);

    dd("calling set_var func for %s", p);

    func = (ndk_set_var_value_pt) lua_touserdata(L, lua_upvalueindex(2));

    rc = func(r, &res, &arg);

    if (rc != NGX_OK) {
        return luaL_error(L, "calling directive %s failed with code %d",
                p, (int) rc);
    }

    lua_pushlstring(L, (char *) res.data, res.len);

    return 1;
}


static ndk_set_var_value_pt
ngx_http_lookup_ndk_set_var_directive(u_char *name,
        size_t name_len)
{
    ndk_set_var_t           *filter;
    ngx_uint_t               i;
    ngx_module_t            *module;
    ngx_command_t           *cmd;

    for (i = 0; ngx_modules[i]; i++) {
        module = ngx_modules[i];
        if (module->type != NGX_HTTP_MODULE) {
            continue;
        }

        cmd = ngx_modules[i]->commands;
        if (cmd == NULL) {
            continue;
        }

        for ( /* void */ ; cmd->name.len; cmd++) {
            if (cmd->set != ndk_set_var_value) {
                continue;
            }

            filter = cmd->post;
            if (filter == NULL) {
                continue;
            }

            if (cmd->name.len != name_len
                    || ngx_strncmp(cmd->name.data, name, name_len) != 0)
            {
                continue;
            }

            return (ndk_set_var_value_pt)(filter->func);
        }
    }

    return NULL;
}

#endif /* defined(NDK) && NDK */
