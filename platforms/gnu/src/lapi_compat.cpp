
extern "C" {
#include <sys/stat.h>
}

#include <lua.h>
#include <blt/lapi_compat.hh>

using namespace blt;

namespace blt
{
	namespace compat
	{
		namespace SystemFS
		{
			int
			exists(lua_state* state)
			{
				// Check arguments
				if(lua_gettop(state) != 2)
					luaL_error(state, "SystemFS:exists(path) takes a single argument, not %d arguments including SystemFS", lua_gettop(state));
				if(!lua_isstring(state, -1))
					luaL_error(state, "SystemFS:exists(path) -> argument 'path' must be a string!");

				// assuming PWD is base folder
				const char* path = lua_tolstring(state, -1, NULL);
				struct stat _stat;
				lua_pushboolean(state, stat(path, &_stat) == 0);
				return 1;
			}
		}

		void add_members(lua_state* state)
		{
			luaL_Reg lib_SystemFS[] =
			{
				{ "exists",         SystemFS::exists        },
				{ NULL, NULL }
			};
			luaL_openlib(state, "SystemFS", lib_SystemFS, 0);
		}
	}
}

/* vim: set ts=4 softtabstop=0 sw=4 expandtab: */
