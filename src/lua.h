#pragma once

#include <stdint.h>

#ifdef _WIN32
#include "../platforms/w32/signatures/sigdef.h"
#elif __GNUC__
#include "../platforms/gnu/include/lua.hh"
#else
#error Unknown platform - either _WIN32 or __GNUC__ must be defined
#endif

inline uint64_t luaX_toidstring(lua_State *L, int index)
{
	return *(uint64_t*) lua_touserdata(L, index);
}
