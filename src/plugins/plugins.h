#pragma once
#include <lua.h>
#include <string>
#include <list>

#ifdef WIN32
#include <windows.h>
#endif

namespace blt
{
	namespace plugins
	{
		typedef void(*update_func_t)(lua_State *L);
		typedef void(*setup_state_func_t)(lua_State *L);

		class Plugin
		{
		public:
			Plugin(std::string file);
			Plugin(Plugin&) = delete;

			void AddToState(lua_State *L);

			void Update(lua_State *L);

			const std::string GetFile() const
			{
				return file;
			}
		private:
			std::string file;

			update_func_t update_func;
			setup_state_func_t setup_state;

			// Native variable storage:
#ifdef WIN32
			HMODULE module;
#elif __GNUC__
			void *dlhandle;
#endif
		};

		enum PluginLoadResult
		{
			plr_Success,
			plr_AlreadyLoaded
		};

		PluginLoadResult LoadPlugin(std::string file);
		const std::list<Plugin*> &GetPlugins();

		// Implemented in InitiateState
		// TODO find a cleaner solution
		void RegisterPluginForActiveStates(Plugin *plugin);
	}
}
