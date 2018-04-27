#pragma once
#include <lua.h>
#include <string>

#ifdef WIN32
#include <windows.h>
#endif

namespace blt {
	namespace plugins {
		class Plugin {
		public:
			Plugin(std::string file);

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
	}
}
