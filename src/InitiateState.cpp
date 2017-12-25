#include "InitState.h"
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <detours.h>

#define SIG_INCLUDE_MAIN
#include "signatures/sigdef.h"
#undef SIG_INCLUDE_MAIN

#include "util/util.h"
#include "console/console.h"
#include "threading/queue.h"
#include "http/http.h"
#include "vr/vr.h"
#include "debug/debug.h"
#include "xaudio/xaudio.h"
#include "tweaker/xmltweaker.h"

#include <thread>
#include <list>
#include <fstream>

// Code taken from LuaJIT 2.1.0-beta2
namespace pd2hook
{

	std::list<lua_State*> activeStates;
	void add_active_state(lua_State* L)
	{
		activeStates.push_back(L);
	}

	void remove_active_state(lua_State* L)
	{
		activeStates.remove(L);
	}

	bool check_active_state(lua_State* L)
	{
		std::list<lua_State*>::iterator it;
		for (it = activeStates.begin(); it != activeStates.end(); it++)
		{
			if (*it == L)
			{
				return true;
			}
		}
		return false;
	}

	FuncDetour* luaCallDetour = nullptr;

	// Not tracking the error count here so it can automatically be reset to 0 whenever the Lua state is deleted and re-created (e.g.
	// when transitioning to / from the menu to a level)
	void NotifyErrorOverlay(lua_State* L, const char* message)
	{
		lua_getglobal(L, "NotifyErrorOverlay");
		if (lua_isfunction(L, -1))
		{
			int args = 0;
			if (message)
			{
				lua_pushstring(L, message);
				args = 1;
			}
			int error = lua_pcall(L, args, 0, 0);
			if (error == LUA_ERRRUN)
			{
				// Don't bother logging the error since the error overlay is designed to be an optional component, just pop the error
				// message off the stack to keep it balanced
				lua_pop(L, 1);
				return;
			}
		}
		else
		{
			lua_pop(L, 1);
			static bool printed = false;
			if (!printed)
			{
				printf("Warning: Failed to find the NotifyErrorOverlay function in the Lua environment; no in-game notifications will be displayed for caught errors\n");
				printed = true;
			}
		}
	}

	void lua_newcall(lua_State* L, int args, int returns)
	{
		// https://stackoverflow.com/questions/30021904/lua-set-default-error-handler/30022216#30022216
		lua_getglobal(L, "debug");
		lua_getfield(L, -1, "traceback");
		lua_remove(L, -2);
		// Do not index from the top (i.e. use a negative index) as this has the potential to mess up if the callee function returns
		// values /and/ lua_pcall() is set up with a > 0 nresults argument
		int errorhandler = lua_gettop(L) - args - 1;
		lua_insert(L, errorhandler);

		int result = lua_pcall(L, args, returns, errorhandler);
		if (result != 0)
		{
			size_t len;
			const char* message = lua_tolstring(L, -1, &len);
			PD2HOOK_LOG_ERROR(message);
			NotifyErrorOverlay(L, message);
			// This call pops the error message off the stack
			lua_pop(L, 1);
			// No, don't touch this variable anymore
			message = nullptr;
		}
		// This call removes the error handler from the stack. Do not use lua_pop() as the callee function's return values may be
		// present, which would pop one of those instead and leave the error handler on the stack
		lua_remove(L, errorhandler);
	}

	int luaF_ispcallforced(lua_State* L)
	{
		lua_pushboolean(L, luaCallDetour ? true : false);
		return 1;
	}

	int luaF_forcepcalls(lua_State* L)
	{
		int args = lua_gettop(L);	// Number of arguments
		if (args < 1)
		{
			PD2HOOK_LOG_WARN("blt.forcepcalls(): Called with no arguments, ignoring request");
			return 0;
		}

		if (lua_toboolean(L, 1))
		{
			if (!luaCallDetour)
			{
				luaCallDetour = new FuncDetour((void**)&lua_call, lua_newcall);
				PD2HOOK_LOG_LOG("blt.forcepcalls(): Protected calls will now be forced");
			}
			//		else Logging::Log("blt.forcepcalls(): Protected calls are already being forced, ignoring request", Logging::LOGGING_WARN);
		}
		else
		{
			if (luaCallDetour)
			{
				delete luaCallDetour;
				luaCallDetour = nullptr;
				PD2HOOK_LOG_LOG("blt.forcepcalls(): Protected calls are no longer being forced");
			}
			//		else Logging::Log("blt.forcepcalls(): Protected calls are not currently being forced, ignoring request", Logging::LOGGING_WARN);
		}
		return 0;
	}

	bool vrMode = false;

	int luaF_getvrstate(lua_State* L) {
		lua_pushboolean(L, vrMode);
		return 1;
	}

	int luaF_isvrhookloaded(lua_State* L) {
		lua_pushboolean(L, VRManager::GetInstance()->IsLoaded());
		return 1;
	}

	int luaF_gethmdbrand(lua_State* L) {
		std::string hmd = VRManager::GetInstance()->GetHMDBrand();
		lua_pushstring(L, hmd.c_str());
		return 1;
	}

	int luaF_getbuttonstate(lua_State* L) {
		int id = lua_tonumber(L, -1);
		lua_remove(L, -1);
		lua_pushinteger(L, VRManager::GetInstance()->GetButtonsStatus(id));
		return 1;
	}

	int luaH_getcontents(lua_State* L, bool files)
	{
		size_t len;
		const char* dirc = lua_tolstring(L, 1, &len);
		std::string dir(dirc, len);
		std::vector<std::string> directories;

		try
		{
			directories = Util::GetDirectoryContents(dir, files);
		}
		catch (int unused)
		{
			// Okay, okay - now shut up, compiler
			(void)unused;
			lua_pushboolean(L, false);
			return 1;
		}

		lua_createtable(L, 0, 0);

		std::vector<std::string>::iterator it;
		int index = 1;
		for (it = directories.begin(); it < directories.end(); it++)
		{
			if (*it == "." || *it == "..") continue;
			lua_pushinteger(L, index);
			lua_pushlstring(L, it->c_str(), it->length());
			lua_settable(L, -3);
			index++;
		}

		return 1;
	}

	int luaF_getdir(lua_State* L)
	{
		return luaH_getcontents(L, true);
	}

	int luaF_getfiles(lua_State* L)
	{
		return luaH_getcontents(L, false);
	}

	int luaF_directoryExists(lua_State* L)
	{
		size_t len;
		const char* dirc = lua_tolstring(L, 1, &len);
		bool doesExist = Util::DirectoryExists(dirc);
		lua_pushboolean(L, doesExist);
		return 1;
	}

	int luaF_unzipfile(lua_State* L)
	{
		size_t len;
		const char* archivePath = lua_tolstring(L, 1, &len);
		const char* extractPath = lua_tolstring(L, 2, &len);

		pd2hook::ExtractZIPArchive(archivePath, extractPath);
		return 0;
	}

	int luaF_removeDirectory(lua_State* L)
	{
		size_t len;
		const char* directory = lua_tolstring(L, 1, &len);
		bool success = Util::RemoveEmptyDirectory(directory);
		lua_pushboolean(L, success);
		return 1;
	}

	int luaF_pcall(lua_State* L)
	{
		int args = lua_gettop(L) - 1;

		lua_getglobal(L, "debug");
		lua_getfield(L, -1, "traceback");
		lua_remove(L, -2);
		// Do not index from the top (i.e. don't use a negative index)
		int errorhandler = lua_gettop(L) - args - 1;
		lua_insert(L, errorhandler);

		int result = lua_pcall(L, args, LUA_MULTRET, errorhandler);
		// lua_pcall() automatically pops the callee function and its arguments off the stack. Then, if no errors were encountered
		// during execution, it pushes the return values onto the stack, if any. Otherwise, if an error was encountered, it pushes
		// the error message onto the stack, which should manually be popped off when done using to keep the stack balanced
		if (result == LUA_ERRRUN)
		{
			size_t len;
			PD2HOOK_LOG_ERROR(lua_tolstring(L, -1, &len));
			// This call pops the error message off the stack
			lua_pop(L, 1);
			return 0;
		}
		// Do not use lua_pop() as the callee function's return values may be present, which would pop one of those instead and leave
		// the error handler on the stack
		lua_remove(L, errorhandler);
		lua_pushboolean(L, result == 0);
		lua_insert(L, 1);

		//if (result != 0) return 1;

		return lua_gettop(L);
	}

	int luaF_dofile(lua_State* L)
	{

		int n = lua_gettop(L);

		size_t length = 0;
		const char* filename = lua_tolstring(L, 1, &length);
		int error = luaL_loadfilex(L, filename, nullptr);
		if (error != 0)
		{
			size_t len;
			//		Logging::Log(filename, Logging::LOGGING_ERROR);
			PD2HOOK_LOG_ERROR(lua_tolstring(L, -1, &len));
		}
		else
		{
			lua_getglobal(L, "debug");
			lua_getfield(L, -1, "traceback");
			lua_remove(L, -2);
			// Example stack for visualization purposes:
			// 3 debug.traceback
			// 2 compiled code as a self-contained function
			// 1 filename
			// Do not index from the top (i.e. don't use a negative index)
			int errorhandler = 2;
			lua_insert(L, errorhandler);

			error = lua_pcall(L, 0, 0, errorhandler);
			if (error == LUA_ERRRUN)
			{
				size_t len;
				//			Logging::Log(filename, Logging::LOGGING_ERROR);
				PD2HOOK_LOG_ERROR(lua_tolstring(L, -1, &len));
				// This call pops the error message off the stack
				lua_pop(L, 1);
			}
			// Do not use lua_pop() as the callee function's return values may be present, which would pop one of those instead and
			// leave the error handler on the stack
			lua_remove(L, errorhandler);
		}
		return 0;
	}

	struct lua_http_data
	{
		int funcRef;
		int progressRef;
		int requestIdentifier;
		lua_State* L;
	};

	void return_lua_http(void* data, std::string& urlcontents)
	{
		lua_http_data* ourData = (lua_http_data*)data;
		if (!check_active_state(ourData->L))
		{
			delete ourData;
			return;
		}

		lua_rawgeti(ourData->L, LUA_REGISTRYINDEX, ourData->funcRef);
		lua_pushlstring(ourData->L, urlcontents.c_str(), urlcontents.length());
		lua_pushinteger(ourData->L, ourData->requestIdentifier);
		lua_pcall(ourData->L, 2, 0, 0);
		luaL_unref(ourData->L, LUA_REGISTRYINDEX, ourData->funcRef);
		luaL_unref(ourData->L, LUA_REGISTRYINDEX, ourData->progressRef);
		delete ourData;
	}

	void progress_lua_http(void* data, long progress, long total)
	{
		lua_http_data* ourData = (lua_http_data*)data;

		if (!check_active_state(ourData->L))
		{
			return;
		}

		if (ourData->progressRef == 0) return;
		lua_rawgeti(ourData->L, LUA_REGISTRYINDEX, ourData->progressRef);
		lua_pushinteger(ourData->L, ourData->requestIdentifier);
		lua_pushinteger(ourData->L, progress);
		lua_pushinteger(ourData->L, total);
		lua_pcall(ourData->L, 3, 0, 0);
	}

	int luaF_directoryhash(lua_State* L)
	{
		PD2HOOK_TRACE_FUNC;
		int n = lua_gettop(L);

		size_t length = 0;
		const char* filename = lua_tolstring(L, 1, &length);
		std::string hash = Util::GetDirectoryHash(filename);
		lua_pushlstring(L, hash.c_str(), hash.length());

		return 1;
	}

	int luaF_filehash(lua_State* L)
	{
		int n = lua_gettop(L);
		size_t l = 0;
		const char * fileName = lua_tolstring(L, 1, &l);
		std::string hash = Util::GetFileHash(fileName);
		lua_pushlstring(L, hash.c_str(), hash.length());
		return 1;
	}

	static int HTTPReqIdent = 0;

	int luaF_dohttpreq(lua_State* L)
	{
		PD2HOOK_LOG_LOG("Incoming HTTP Request/Request");

		int args = lua_gettop(L);
		int progressReference = 0;
		if (args >= 3)
		{
			progressReference = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		int functionReference = luaL_ref(L, LUA_REGISTRYINDEX);
		size_t len;
		const char* url_c = lua_tolstring(L, 1, &len);
		std::string url = std::string(url_c, len);

		PD2HOOK_LOG_LOG(std::string(url_c, len) << " - " << functionReference);

		lua_http_data* ourData = new lua_http_data();
		ourData->funcRef = functionReference;
		ourData->progressRef = progressReference;
		ourData->L = L;

		HTTPReqIdent++;
		ourData->requestIdentifier = HTTPReqIdent;

		std::unique_ptr<HTTPItem> reqItem(new HTTPItem());
		reqItem->call = return_lua_http;
		reqItem->data = ourData;
		reqItem->url = url;

		if (progressReference != 0)
		{
			reqItem->progress = progress_lua_http;
		}

		HTTPManager::GetSingleton()->LaunchHTTPRequest(std::move(reqItem));
		lua_pushinteger(L, HTTPReqIdent);
		return 1;
	}

	CConsole* gbl_mConsole = NULL;

	int luaF_createconsole(lua_State* L)
	{
		if (gbl_mConsole) return 0;
		gbl_mConsole = new CConsole();
		return 0;
	}

	int luaF_destroyconsole(lua_State* L)
	{
		if (!gbl_mConsole) return 0;
		delete gbl_mConsole;
		gbl_mConsole = NULL;
		return 0;
	}

	int luaF_print(lua_State* L)
	{
		int top = lua_gettop(L);
		std::stringstream stream;
		for (int i = 0; i < top; ++i)
		{
			size_t len;
			const char* str = lua_tolstring(L, i + 1, &len);
			stream << (i > 0 ? "    " : "") << str;
		}
		PD2HOOK_LOG_LUA(stream.str());

		return 0;
	}

	int luaF_moveDirectory(lua_State * L)
	{
		int top = lua_gettop(L);
		size_t lf = 0;
		const char * fromStr = lua_tolstring(L, 1, &lf);
		size_t ld = 0;
		const char * destStr = lua_tolstring(L, 2, &ld);

		bool success = Util::MoveDirectory(fromStr, destStr);
		lua_pushboolean(L, success);
		return 1;
	}

	int luaF_createDirectory(lua_State * L)
	{
		const char *path = lua_tostring(L, 1);
		bool success = CreateDirectoryA(path, NULL);
		lua_pushboolean(L, success);
		return 1;
	}

	void load_vr_globals(lua_State *L)
	{
		luaL_Reg vrLib[] = {
			{ "getvrstate", luaF_getvrstate },
			{ "isvrhookloaded", luaF_isvrhookloaded },
			{ "gethmdbrand", luaF_gethmdbrand },
			{ "getbuttonstate", luaF_getbuttonstate },
			{ NULL, NULL }
		};
		luaI_openlib(L, "blt_vr", vrLib, 0);
	}

	int updates = 0;
	std::thread::id main_thread_id;

	void* __fastcall do_game_update_new(void* thislol, int edx, int* a, int* b)
	{

		// If someone has a better way of doing this, I'd like to know about it.
		// I could save the this pointer?
		// I'll check if it's even different at all later.
		if (std::this_thread::get_id() != main_thread_id)
		{
			return do_game_update(thislol, a, b);
		}

		lua_State* L = (lua_State*)*((void**)thislol);
		if (updates == 0)
		{
			HTTPManager::GetSingleton()->init_locks();
		}

		if (updates > 1)
		{
			EventQueueMaster::GetSingleton().ProcessEvents();
		}

		DebugConnection::Update(L);

		updates++;
		return do_game_update(thislol, a, b);
	}

	bool setup_check_done = false;

	// Random dude who wrote what's his face?
	// I 'unno, I stole this method from the guy who wrote the 'underground-light-lua-hook'
	// Mine worked fine, but this seems more elegant.
	int __fastcall luaL_newstate_new(void* thislol, int edx, char no, char freakin, int clue)
	{
		int ret = (vrMode ? luaL_newstate_vr : luaL_newstate)(thislol, no, freakin, clue);

		lua_State* L = (lua_State*)*((void**)thislol);
		printf("Lua State: %p\n", (void*)L);
		if (!L) return ret;

		add_active_state(L);

		if (!setup_check_done) {
			setup_check_done = true;

			if (!(Util::DirectoryExists("mods") && Util::DirectoryExists("mods/base"))) {
				int result = MessageBox(NULL, "Do you want to download the PAYDAY 2 BLT basemod?\n"
					"This is required for using mods", "BLT 'mods/base' folder missing", MB_YESNO);
				if (result == IDYES) download_blt();

				return ret;
			}
		}

		lua_pushcclosure(L, luaF_print, 0);
		lua_setfield(L, LUA_GLOBALSINDEX, "log");

		lua_pushcclosure(L, luaF_pcall, 0);
		lua_setfield(L, LUA_GLOBALSINDEX, "pcall");

		lua_pushcclosure(L, luaF_dofile, 0);
		lua_setfield(L, LUA_GLOBALSINDEX, "dofile");

		lua_pushcclosure(L, luaF_unzipfile, 0);
		lua_setfield(L, LUA_GLOBALSINDEX, "unzip");

		lua_pushcclosure(L, luaF_dohttpreq, 0);
		lua_setfield(L, LUA_GLOBALSINDEX, "dohttpreq");

		luaL_Reg consoleLib[] = {
			{ "CreateConsole", luaF_createconsole },
			{ "DestroyConsole", luaF_destroyconsole },
			{ NULL, NULL }
		};
		luaI_openlib(L, "console", consoleLib, 0);

		luaL_Reg fileLib[] = {
			{ "GetDirectories", luaF_getdir },
			{ "GetFiles", luaF_getfiles },
			{ "RemoveDirectory", luaF_removeDirectory },
			{ "DirectoryExists", luaF_directoryExists },
			{ "DirectoryHash", luaF_directoryhash },
			{ "FileHash", luaF_filehash },
			{ "MoveDirectory", luaF_moveDirectory },
			{ "CreateDirectory", luaF_createDirectory },
			{ NULL, NULL }
		};
		luaI_openlib(L, "file", fileLib, 0);

		// Keeping everything in lowercase since IspcallForced / IsPCallForced and Forcepcalls / ForcePCalls look rather weird anyway
		luaL_Reg bltLib[] = {
			{ "ispcallforced", luaF_ispcallforced },
			{ "forcepcalls", luaF_forcepcalls },
			{ NULL, NULL }
		};
		luaI_openlib(L, "blt", bltLib, 0);

		if (vrMode)
		{
			load_vr_globals(L);
		}

		DebugConnection::AddGlobals(L);
#ifdef ENABLE_XAUDIO
		XAudio::Register(L);
#endif

		int result;
		PD2HOOK_LOG_LOG("Initiating Hook");

		result = luaL_loadfilex(L, "mods/base/base.lua", nullptr);
		if (result == LUA_ERRSYNTAX)
		{
			size_t len;
			PD2HOOK_LOG_ERROR(lua_tolstring(L, -1, &len));
			return ret;
		}
		result = lua_pcall(L, 0, 1, 0);
		if (result == LUA_ERRRUN)
		{
			size_t len;
			PD2HOOK_LOG_LOG(lua_tolstring(L, -1, &len));
			return ret;
		}

		return ret;
	}

	int __fastcall luaL_newstate_new_vr(void* thislol, int edx, char no, char freakin, int clue) {
		vrMode = true;
		return luaL_newstate_new(thislol, edx, no, freakin, clue);
	}

	void luaF_close(lua_State* L)
	{
		remove_active_state(L);
		lua_close(L);
	}

	void InitiateStates()
	{
		// Set up logging first, so we can see messages from the signature search process
#ifndef INJECTABLE_BLT
		std::ifstream infile("mods/developer.txt");
		std::string debug_mode;
		if (infile.good()) {
			debug_mode = "post"; // default value
			infile >> debug_mode;
		}
		else {
			debug_mode = "disabled";
		}

		if (debug_mode == "pre")
#endif
		gbl_mConsole = new CConsole();

		main_thread_id = std::this_thread::get_id();

		// Set up debugging right away, for log viewing
		DebugConnection::Initialize();

#ifndef INJECTABLE_BLT
		if (debug_mode != "disabled" && gbl_mConsole == NULL && !DebugConnection::IsLoaded())
			gbl_mConsole = new CConsole();
#endif

		SignatureSearch::Search();
		VRManager::CheckAndLoad();

		if (node_from_xml == NULL) node_from_xml = node_from_xml_vr;

		FuncDetour* gameUpdateDetour = new FuncDetour((void**)&do_game_update, do_game_update_new);
		FuncDetour* newStateDetour = new FuncDetour((void**)&luaL_newstate, luaL_newstate_new);
		FuncDetour* newStateDetourVr = new FuncDetour((void**)&luaL_newstate_vr, luaL_newstate_new_vr);
		FuncDetour* luaCloseDetour = new FuncDetour((void**)&lua_close, luaF_close);

		FuncDetour* node_from_xmlDetour = new FuncDetour((void**)&node_from_xml, tweaker::node_from_xml_new);

		tweaker::init_xml_tweaker();
	}

	void DestroyStates()
	{
		// Okay... let's not do that.
		// I don't want to keep this in memory, but it CRASHES THE SHIT OUT if you delete this after all is said and done.
		if (gbl_mConsole) delete gbl_mConsole;
	}

}