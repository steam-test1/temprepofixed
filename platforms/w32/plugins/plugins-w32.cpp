#include "plugins/plugins.h"
#include "util/util.h"
#include "InitState.h"
#include "platform.h"

using namespace std;

// Don't use these funcdefs when porting to GNU+Linux, as it doesn't need
// any kind of getter function because the PD2 binary isn't stripped
typedef void*(*lua_access_func_t)(const char*);
typedef void(*init_func_t)(lua_access_func_t get_lua_func_by_name);

// Do port these
typedef void(*setup_state_func_t)(lua_State *L);
typedef void(*update_func_t)(lua_State *L);

static void pd2_log(const char* message, int level, const char* file, int line)
{
	using LT = pd2hook::Logging::LogType;

	char buffer[256];
	sprintf_s(buffer, sizeof(buffer), "ExtModDLL %s", file);

	switch ((LT)level)
	{
	case LT::LOGGING_FUNC:
	case LT::LOGGING_LOG:
	case LT::LOGGING_LUA:
		PD2HOOK_LOG_LEVEL(message, (LT)level, buffer, line, FOREGROUND_RED, FOREGROUND_BLUE, FOREGROUND_INTENSITY);
		break;
	case LT::LOGGING_WARN:
		PD2HOOK_LOG_LEVEL(message, (LT)level, buffer, line, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_INTENSITY);
		break;
	case LT::LOGGING_ERROR:
		PD2HOOK_LOG_LEVEL(message, (LT)level, buffer, line, FOREGROUND_RED, FOREGROUND_INTENSITY);
		break;
	}
}

static bool is_active_state(lua_State *L)
{
	return pd2hook::check_active_state(L);
}

static void * get_func(const char* name)
{
	string str = name;

	if (str == "pd2_log")
	{
		return &pd2_log;
	}
	else if (str == "is_active_state")
	{
		return &is_active_state;
	}

	return blt::platform::win32::get_lua_func(name);
}

blt::plugins::Plugin::Plugin(std::string file) : file(file)
{
	module = LoadLibraryA(file.c_str());

	if (!module) throw string("Failed to load module: ERR") + to_string(GetLastError());

	// Version compatibility.
	uint64_t *SBLT_API_REVISION = (uint64_t*)GetProcAddress(module, "SBLT_API_REVISION");
	if (!SBLT_API_REVISION) throw string("Missing export SBLT_API_REVISION");

	switch (*SBLT_API_REVISION) {
	case 1:
		// Nothing special for now.
		break;
	default:
		throw string("Unsupported revision ") + to_string(*SBLT_API_REVISION) + " - you probably need to update SuperBLT";
	}

	// Verify the licence compliance.
	const char * const *MODULE_LICENCE_DECLARATION = (const char * const *)GetProcAddress(module, "MODULE_LICENCE_DECLARATION");
	if (!MODULE_LICENCE_DECLARATION) throw string("Licence error: Missing export MODULE_LICENCE_DECLARATION");

	const char * const *MODULE_SOURCE_CODE_LOCATION = (const char * const *)GetProcAddress(module, "MODULE_SOURCE_CODE_LOCATION");
	if (!MODULE_SOURCE_CODE_LOCATION) throw string("Licence error: Missing export MODULE_SOURCE_CODE_LOCATION");

	const char * const *MODULE_SOURCE_CODE_REVISION = (const char * const *)GetProcAddress(module, "MODULE_SOURCE_CODE_REVISION");
	if (!MODULE_SOURCE_CODE_REVISION) throw string("Licence error: Missing export MODULE_SOURCE_CODE_REVISION");

	const char *required_declaration = "This module is licenced under the GNU GPL version 2 or later, or another compatible licence";

	if (strcmp(required_declaration, *MODULE_LICENCE_DECLARATION)) {
		throw string("Invalid licence declaration '") + string(*MODULE_LICENCE_DECLARATION) + string("'");
	}

	bool developer = false;
	if (*MODULE_SOURCE_CODE_LOCATION) {
		// TODO handle this, put it somewhere accessable by Lua.
	}
	else {
		// TODO also make Lua aware of this.
		developer = true;
	}

	// Start loading everything
	init_func_t init = (init_func_t)GetProcAddress(module, "SuperBLT_Plugin_Setup");
	if (!init) throw "Invalid module - missing initfunc!";

	setup_state = GetProcAddress(module, "SuperBLT_Plugin_Init_State");
	if (!setup_state) throw "Invalid module - missing setup_state func!";

	update_func = GetProcAddress(module, "SuperBLT_Plugin_Update");

	init(get_func);

	// TODO add updating (game update call)
}

void blt::plugins::Plugin::AddToState(lua_State * L)
{
	((setup_state_func_t)setup_state)(L);
}

void blt::plugins::Plugin::Update(lua_State * L)
{
	if (update_func)
		((update_func_t)update_func)(L);
}
