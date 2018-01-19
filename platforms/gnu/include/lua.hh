#pragma once

#include <cstddef>

#define LUA_NUMBER double

/* Note: changing the following defines breaks the Lua 5.1 ABI. */
#define LUA_INTEGER  ptrdiff_t
#define LUA_IDSIZE   60 /* Size of lua_Debug.short_src. */

#define LUAL_BUFFERSIZE (BUFSIZ > 16384 ? 8192 : BUFSIZ)

extern "C" {
   #define LUA_API extern
   #define LUALIB_API LUA_API

   #include "pure_lua.h"
   #include "pure_lauxlib.h"

   #undef LUALIB_API
   #undef LUA_API
}

// The windows version of BLT uses lua_State, whereas blt4l uses lua_state. Set up a define to fix that.
#define lua_state lua_State


/* vim: set ts=3 softtabstop=0 sw=3 expandtab: */

