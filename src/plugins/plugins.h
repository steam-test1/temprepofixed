#pragma once
#include <lua.h>
#include <string>
#include <list>

#ifdef WIN32
#include <windows.h>
#endif

namespace blt {
	namespace plugins {
		class Plugin {
		public:
			Plugin(std::string file);
			Plugin(Plugin&) = delete;

			void AddToState(lua_State *L);

			void Update(lua_State *L);

			const std::string GetFile() const { return file; }
		private:
			std::string file;

			// Native variable storage:
#ifdef WIN32
			HMODULE module;
			FARPROC setup_state;
			FARPROC update_func;
#endif
		};

		enum PluginLoadResult {
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
