#pragma once

#include <lua.h>

namespace blt
{
	void init_asset_hook(void *dlHandle);
	void asset_add_lua_members(lua_State *L);
};

/* vim: set ts=4 softtabstop=0 sw=4 expandtab: */

