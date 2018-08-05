#pragma once

#ifdef SIG_INCLUDE_MAIN
#include "signatures.h"

#define CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset);

#define CREATE_CALLABLE_CLASS_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(__thiscall *name ## ptr)(void*, __VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset);

#else

// If we're not being included directly from InitiateState.cpp, only declare, not define, variables
#define CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	extern name ## ptr name;

#define CREATE_CALLABLE_CLASS_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(__thiscall *name ## ptr)(void*, __VA_ARGS__); \
	extern name ## ptr name;

#endif

struct lua_State;

typedef const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);
typedef int(*lua_CFunction) (lua_State *L);
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);
typedef struct luaL_Reg
{
	const char* name;
	lua_CFunction func;
} luaL_Reg;

// From src/luaconf.h
#define LUA_NUMBER		double

// From src/lua.h
// type of numbers in Lua
typedef LUA_NUMBER lua_Number;
typedef struct lua_Debug lua_Debug;	// activation record
// Functions to be called by the debuger in specific events
typedef void(*lua_Hook) (lua_State* L, lua_Debug* ar);

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_call, void, "\x8B\x44\x24\x08\x8B\x54\x24\x04\xFF\x44\x24\x0C\x8D\x0C\xC5\x00", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pcall, int, "\x8B\x54\x24\x04\x8B\x4C\x24\x10\x53\x56\x8B\x72\x08\x8A", "xxxxxxxxxxxxxx", 0, lua_State*, int, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_gettop, int, "\x8B\x4C\x24\x04\x8B\x41\x14\x2B\x41\x10\xC1\xF8\x03\xC3", "xxxxxxxxxxxxxx", 0, lua_State*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_settop, void, "\x8B\x44\x24\x08\x85\xC0\x78\x5B\x53\x56\x8B\x74\x24\x0C\x57\x8B", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_toboolean, int, "\xFF\x74\x24\x08\xFF\x74\x24\x08\xE8\x00\x00\x00\x00\x83\xC4\x08\x83\x78\x04\xFE\x1B\xC0\xF7\xD8\xC3", "xxxxxxxxx????xxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_tointeger, size_t, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\xFF\x75\x0C\xFF\x75\x08\xE8\x00\x00\x00\x00\x8B\x48\x04\x83\xC4\x08\x83\xF9\xF2\x73\x0C\xF2\x0F\x10\x00\xF2\x0F\x2C\xC0\x8B\xE5\x5D\xC3\x83\xF9\xFB\x75\x26", "xxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_tonumber, lua_Number, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\xFF\x75\x0C\xFF\x75\x08\xE8\x00\x00\x00\x00\x8B\x48\x04\x83\xC4\x08\x83\xF9\xF2\x77\x06\xDD", "xxxxxxxxxxxxxxxx????xxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_tolstring, const char*, "\x83\xEC\x24\xA1\x00\x00\x00\x00\x33\xC4\x89\x44\x24\x20\x53\x8B\x5C\x24\x2C\x56\x8B\x74\x24\x34", "xxxx????xxxxxxxxxxxxxxxx", 0, lua_State*, int, size_t*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_objlen, size_t, "\x83\xEC\x24\xA1\x00\x00\x00\x00\x33\xC4\x89\x44\x24\x20\x8B\x44", "xxxx????xxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_touserdata, void*, "\xFF\x74\x24\x08\xFF\x74\x24\x08\xE8\x83\xD2\xFE\xFF******\x83\xF9\xF3\x75\x06\x8B\x00\x83\xC0\x18\xC3\x83\xF9", "xxxxxxxxxxxxx??????xxxxxxxxxxxxx", 0, lua_State*, int)
// This is actually luaL_loadfilex() (as per Lua 5.2) now. The new parameter corresponds to mode, and specifying NULL causes Lua
// to default to "bt", i.e. 'binary and text'
// https://www.lua.org/manual/5.2/manual.html#luaL_loadfilex
// https://www.lua.org/manual/5.2/manual.html#pdf-load
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_loadfilex, int, "\x81\xEC\x08\x02\x00\x00\xA1\x00\x00\x00\x00\x33\xC4\x89\x84\x24", "xxxxxxx????xxxxx", 0, lua_State*, const char*, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_loadstring, int, "\x8B\x54\x24\x08\x83\xEC\x08\x8B\xC2\x56\x8D\x70\x01\x8D\x49\x00", "xxxxxxxxxxxxxxxx", 0, lua_State*, const char*)
//CREATE_NORMAL_CALLABLE_SIGNATURE(lua_load, int, "\x8B\x4C\x24\x10\x33\xD2\x83\xEC\x18\x3B\xCA", "xxxxxxxxxxx", 0, lua_State*, lua_Reader, void*, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_getfield, void, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\x56\x8B\x75\x08\x57\xFF\x75", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_setfield, void, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x10\x56\x8B\x75\x08\x57\xFF\x75", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_createtable, void, "\x56\x8B\x74\x24\x08\x8B\x4E\x08\x8B\x41\x14\x3B\x41\x18\x72\x07\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x44\x24\x10\x85\xC0\x74\x12\x83", "xxxxxxxxxxxxxxxxxxx????xxxxxxxxx", 0, lua_State*, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_newuserdata, void*, "\x56\x8B\x74\x24\x08\x8B\x4E\x08\x8B\x41\x14\x3B\x41\x18\x72\x07\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x4C\x24\x0C\x81\xF9\x00\xFF\xFF", "xxxxxxxxxxxxxxxxxxx????xxxxxxxxx", 0, lua_State*, size_t)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_insert, void, "\x8B\x4C\x24\x08\x56\x57\x85\xC9\x7E\x21\x8B\x54\x24\x0C\x8D\x71", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)
// Missing? We don't need it anyway.
//CREATE_NORMAL_CALLABLE_SIGNATURE(lua_replace, void, "\x56\x57\x8B\x7C\x24\x10\x81\xFF\xEE\xD8\xFF\xFF\x75\x16\x8B\x4C\x24\x0C\x5F\x8B\x41\x14\x8D\x71\x14\x8B\x40\xF8\x83\x06\xF8\x89", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_remove, void, "\x8B\x4C\x24\x08\x56\x57\x85\xC9\x7E\x21\x8B\x7C\x24\x0C\x8B\x47\x10\x8B\x57\x14\x8D\x77\x14\x8D\x04\xC8\x83\xC0\xF8\x3B\xC2\x72", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_newstate, lua_State*, "\x53\x8B\x5C\x24\x0C\x55\x8B\x6C\x24\x0C\x56\x57\x68\x40\x10\x00\x00\x6A\x00\x6A\x00\x53\xFF\xD5\x8B\xF0\x83\xC4\x10\x8D\x7E\x30", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_Alloc, void*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_close, void, "\x8B\x44\x24\x04\x53\x56\x57\x8B\x78\x08\x8B\x77\x74\x56\xE8", "xxxxxxxxxxxxxxx", 0, lua_State*)

//CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawset, void, "\x51\x53\x55\x56\x57\x8B\xF1\xE8\x00\x00\x00\x00", "xxxxxxxx????", 0, lua_State*, int)
// Reviving lua_settable() since the function exists again, and because the Crimefest 2015 alternative relied upon internal Lua
// VM functions, which do not apply to LuaJIT
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_settable, void, "\x56\xFF\x74\x24\x0C\x8B\x74\x24\x0C\x56\xE8\x00\x00\x00\x00\x8B\x4E\x14\x83\xE9\x10\x51\x50\x56", "xxxxxxxxxxx????xxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_setmetatable, int, "\x53\x55\x56\x57\xFF\x74\x24\x18\x8B\x7C\x24\x18\x57\xE8\x00\x00\x00\x00\x8B\x77\x14\x83\xC4\x08", "xxxxxxxxxxxxxx????xxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_getmetatable, int, "\x56\xFF\x74\x24\x0C\x8B\x74\x24\x0C\x56\xE8\x00\x00\x00\x00\x8B\x48\x04\x83\xC4\x08\x83\xF9\xF4\x75\x07\x8B\x00\x8B\x48\x10\xEB", "xxxxxxxxxxx????xxxxxxxxxxxxxxxxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushnumber, void, "\x8B\x4C\x24\x04\xF2\x0F\x10\x44\x24\x08\x8B\x41\x14\xF2\x0F\x11\x00\x8B\x51\x14\xF2\x0F\x10\x02\x66\x0F\x2E\xC0\x9F\xF6\xC4\x44", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, lua_Number)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushinteger, void, "\x66\x0F\x6E\x44\x24\x08\x8B\x4C\x24\x04\xF3\x0F\xE6\xC0\x8B\x41", "xxxxxxxxxxxxxxxx", 0, lua_State*, size_t)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushboolean, void, "\x8B\x4C\x24\x04\x33\xC0\x39\x44\x24\x08\xBA\xFE\xFF\xFF\xFF\x0F", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushcclosure, void, "\x56\x8B\x74\x24\x08\x8B\x4E\x08\x8B\x41\x14\x3B\x41\x18\x72\x07\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x46\x10\x8B\x40\xF8\x80\x78\x05", "xxxxxxxxxxxxxxxxxxx????xxxxxxxxx", 0, lua_State*, lua_CFunction, int);
// lua_pushstring()'s signature was found before lua_pushlstring()'s, so I'm leaving it here now since it's valid anyway
// It was used as a quick and dirty - and broken - workaround since most lua_pushlstring() calls are inlined, but it ended up
// breaking HTTP downloads of zip archives due to its sensitivity to premature null characters. A non-inlined signature for
// lua_pushlstring() was found by cross-referencing the string 'loaders' to lj_cf_package_require(), which is part of LuaJIT
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushlstring, void, "\x56\x8B\x74\x24\x08\x8B\x4E\x08\x8B\x41\x14\x3B\x41\x18\x72\x07\x8B\xCE\xE8\x00\x00\x00\x00\xFF", "xxxxxxxxxxxxxxxxxxx????x", 0, lua_State*, const char*, size_t)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushstring, void, "\x56\x8B\x74\x24\x08\x57\x8B\x7C\x24\x10\x85\xFF\x75\x0C\x8B\x46", "xxxxxxxxxxxxxxxx", 0, lua_State*, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_checkstack, int, "\x8B\x54\x24\x08\x81\xFA\x40\x1F\x00\x00\x7F\x38\x8B\x4C\x24\x04", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushvalue, void, "\x56\x57\xFF\x74\x24\x10\x8B\x7C\x24\x10\x57\xE8\x00\x00\x00\x00\x8B\x10\x8B\x77\x14\x83\xC4\x08\x89\x16\x8B\x40\x04\x89\x46\x04", "xxxxxxxxxxxx????xxxxxxxxxxxxxxxx", 0, lua_State*, int)

// luaI_openlib() is really luaL_openlib(), see lauxlib.h in Lua 5.1's source code
CREATE_NORMAL_CALLABLE_SIGNATURE(luaI_openlib, void, "\x55\x8B\xEC\x83\xE4\xF8\xF2\x0F\x10\x00\x00\x00\x00\x00\x83\xEC\x08\x56\x8B\x75\x08\x57\x8B\x46\x14\xF2\x0F", "xxxxxxxxx?????xxxxxxxxxxxxx", 0, lua_State*, const char*, const luaL_Reg*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_ref, int, "\x55\x8B\xEC\x83\xE4\xF8\x8B\x55\x0C\x83\xEC\x08\x8D\x82\x0F\x27\x00\x00\x56\x8B\x75\x08\x57", "xxxxxxxxxxxxxxxx", 0, lua_State*, int);
// Reviving lua_rawgeti() since the function exists again, and because the Crimefest 2015 alternative relied upon internal Lua VM
// functions, which do not apply to LuaJIT
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawgeti, void, "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x56\xFF\x75\x0C\x8B\x75\x08\x56\xE8", "xxxxxxxxxxxxxxxxxx", 0, lua_State*, int, int);
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawseti, void, "\x53\x56\x8B\x74\x24\x0C\x57\xFF\x74\x24\x14\x56\xE8\x00\x00\x00\x00\x8B\x38\x8B", "xxxxxxxxxxxxx????xxx", 0, lua_State*, int, int);
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_type, int, "\x56\xFF\x74\x24\x0C\x8B\x74\x24\x0C\x56\xE8\x00\x00\x00\x00\x8B\xD0\x83\xC4\x08\x8B\x4A\x04\x83\xF9\xF2\x77\x07\xB8\x03\x00\x00\x00\x5E\xC3\x8B\x46\x08\x05\x90", "xxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int);
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_typename, const char*, "\x8B\x44\x24\x08\x8B\x00\x00\x00\x00\x00\x00\xC3\xCC", "xxxxx??????xx", 0, lua_State*, int);
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_unref, void, "\x53\x8B\x5C\x24\x10\x85\xDB\x78\x67\x56\x8B\x74\x24\x0C\x57\x8B", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, int);
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_equal, int, "\x56\x8B\x74\x24\x08\x57\xFF\x74\x24\x10\x56\xE8\x00\x00\x00\x00\xFF\x74\x24\x1C\x8B\xF8\x56\xE8\x00\x00\x00\x00\x8B\x57\x04\x83", "xxxxxxxxxxxx????xxxxxxxx????xxxx", 0, lua_State*, int, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_newmetatable, int, "\x8B\x54\x24\x08\x53\x56\x8B\x74\x24\x0C\x8B\xCA\x8B\x46\x08\x57", "xxxxxxxxxxxxxxxx", 0, lua_State*, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_error, int, "\x8D\x44\x24\x0C\x50\xFF\x74\x24\x0C\xFF\x74\x24\x0C\xE8\x00\x00\x00\x00\x83\xC4\x0C\x50\xFF\x74\x24\x08\xE8", "xxxxxxxxxxxxxx????xxxxxxxxx", 0, lua_State*, const char*, ...)

// TODO: Find address-less signatures
CREATE_NORMAL_CALLABLE_SIGNATURE(node_from_xml, void, "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x28\x53\x33", "xxxxxxxxx????xxxxxxxxxxxxxxxxxxx", 0, void*, char*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(node_from_xml_vr, void, "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x28\x53\x56\x57\x8B\xDA\x8B\xF9\xC7\x44\x24\x18\x00\x00\x00\x00\xC7\x44\x24\x1C\x00\x00\x00\x00\xC7\x44\x24\x20\x00\x00\x00\x00", "xxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, void*, char*, int)

// FIXME This isn't really actually a function - it's a blob that contains references to some of the variables we want.
CREATE_NORMAL_CALLABLE_SIGNATURE(try_open_base, int, "\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x81\xEC\x8C\x00\x00\x00\x8B\x45\x14\xA3\x00\x00\x00\x00\x8B\x45\x18\xA3\x00\x00\x00\x00", "x????x????xxxxxxxxxx????xxxx????", 0)
CREATE_NORMAL_CALLABLE_SIGNATURE(try_open_base_vr, int, "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x90\x00\x00\x00\x8B\x84\x24\xAC\x00", "xxx????xxxxxxxxxxxxxxxxxxxxxxxxx", 0)

CREATE_CALLABLE_CLASS_SIGNATURE(do_game_update, void*, "\x56\xFF\x74\x24\x0C\x8B\xF1\x68\x00\x00\x00\x00\xFF\x36\xE8", "xxxxxxxx????xxx", 0, int*, int*)
CREATE_CALLABLE_CLASS_SIGNATURE(luaL_newstate, int, "\x53\x56\x33\xDB\x57\x8B\xF1\x39\x5C\x24\x18\x0F", "xxxxxxxxxxxx", 0, char, char, int)
CREATE_CALLABLE_CLASS_SIGNATURE(luaL_newstate_vr, int, "\x8B\x44\x24\x0C\x56\x8B\xF1\x85\xC0\x75\x08\x50\x68", "xxxxxxxxxxxxx", 0, char, char, int)

// lua c-functions

// From src/lua.h
// Pseudo-indices
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))

// From src/lauxlib.h
#define LUA_NOREF       (-2)
#define LUA_REFNIL      (-1)

// more bloody lua shit
// Thread status; 0 is OK
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5
// From src/lauxlib.h
// Extra error code for 'luaL_load'
#define LUA_ERRFILE     (LUA_ERRERR+1)

// From src/lua.h
// Option for multiple returns in 'lua_pcall' and 'lua_call'
#define LUA_MULTRET	(-1)
#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

#define lua_pop(L,n)		lua_settop(L, -(n)-1)
#define lua_newtable(L)		lua_createtable(L, 0, 0)
#define lua_isfunction(L,n)	(lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L,n)	(lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)		(lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L,n)	(lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L,n)	(lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L,n)		(lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n)	(lua_type(L, (n)) <= 0)
#define lua_getglobal(L,s)	lua_getfield(L, LUA_GLOBALSINDEX, (s))
#define lua_setglobal(L,s)	lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_tostring(L,i)	lua_tolstring(L, (i), NULL)

#define luaL_getmetatable(L,n)		(lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_openlib luaI_openlib
