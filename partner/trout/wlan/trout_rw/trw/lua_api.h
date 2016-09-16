#ifndef _LUA_API_TROUT_H
#define _LUA_API_TROUT_H
#ifdef  __cplusplus
extern "C" {
#endif
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
LUAMOD_API int luaopen_trw (lua_State *L);

#ifdef  __cplusplus
}
#endif
#endif
