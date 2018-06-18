#include <dlfcn.h>
#include <string.h>
#include <stdio.h>

#include <string>

#include "plugins/plugins.h"
#include "util/util.h"
#include "InitState.h"

using namespace std;
using namespace blt::plugins;

typedef void (*init_func_t)();

// Export this same as the Windows one, for sake
// of compatibility
void superblt_export_unix_logger(const char* message, int level, const char* file, int line)
{
	using LT = pd2hook::Logging::LogType;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "ExtModSO %s", file);

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

// TODO move common logic somewhere
blt::plugins::Plugin::Plugin(std::string file) : file(file)
{
	dlhandle = dlopen(file.c_str(), RTLD_LAZY);

	if (!dlhandle) throw string("Failed to load dlhandle: ERR") + dlerror();

	// Version compatibility.
	uint64_t *SBLT_API_REVISION = (uint64_t*) dlsym(dlhandle, "SBLT_API_REVISION");
	if (!SBLT_API_REVISION) throw string("Missing export SBLT_API_REVISION");

	switch (*SBLT_API_REVISION)
	{
	case 1:
		// Nothing special for now.
		break;
	default:
		throw string("Unsupported revision ") + to_string(*SBLT_API_REVISION) + " - you probably need to update SuperBLT";
	}

	// Verify the licence compliance.
	const char * const *MODULE_LICENCE_DECLARATION = (const char * const *)dlsym(dlhandle, "MODULE_LICENCE_DECLARATION");
	if (!MODULE_LICENCE_DECLARATION) throw string("Licence error: Missing export MODULE_LICENCE_DECLARATION");

	const char * const *MODULE_SOURCE_CODE_LOCATION = (const char * const *)dlsym(dlhandle, "MODULE_SOURCE_CODE_LOCATION");
	if (!MODULE_SOURCE_CODE_LOCATION) throw string("Licence error: Missing export MODULE_SOURCE_CODE_LOCATION");

	const char * const *MODULE_SOURCE_CODE_REVISION = (const char * const *)dlsym(dlhandle, "MODULE_SOURCE_CODE_REVISION");
	if (!MODULE_SOURCE_CODE_REVISION) throw string("Licence error: Missing export MODULE_SOURCE_CODE_REVISION");

	const char *required_declaration = "This dlhandle is licenced under the GNU GPL version 2 or later, or another compatible licence";

	if (strcmp(required_declaration, *MODULE_LICENCE_DECLARATION))
	{
		throw string("Invalid licence declaration '") + string(*MODULE_LICENCE_DECLARATION) + string("'");
	}

	bool developer = false;
	if (*MODULE_SOURCE_CODE_LOCATION)
	{
		// TODO handle this, put it somewhere accessable by Lua.
	}
	else
	{
		// TODO also make Lua aware of this.
		developer = true;
	}

	if(developer)
	{
		PD2HOOK_LOG_WARN("Loading development plugin! This should never occur ourside a development enviornment");
	}

	// Start loading everything
	init_func_t init = (init_func_t)dlsym(dlhandle, "SuperBLT_Plugin_Setup");
	if (!init) throw "Invalid dlhandle - missing initfunc!";

	setup_state = (setup_state_func_t) dlsym(dlhandle, "SuperBLT_Plugin_Init_State");
	if (!setup_state) throw "Invalid dlhandle - missing setup_state func!";

	update_func = (update_func_t) dlsym(dlhandle, "SuperBLT_Plugin_Update");

	init();
}

void blt::plugins::Plugin::AddToState(lua_State * L)
{
	setup_state(L);
}

void blt::plugins::Plugin::Update(lua_State * L)
{
	if (update_func)
		update_func(L);
}

