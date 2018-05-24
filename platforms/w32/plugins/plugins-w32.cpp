#include "plugins/plugins.h"
#include "util/util.h"
#include "InitState.h"
#include "platform.h"

using namespace std;

// Don't use these funcdefs when porting to GNU+Linux, as it doesn't need
// any kind of getter function because the PD2 binary isn't stripped
typedef void*(*lua_access_func_t)(const char*);
typedef void(*init_func_t)(lua_access_func_t get_lua_func_by_name);
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

	// TODO verify signature
	// We MUST do this before publicly releasing a version, as there are major security implications

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
