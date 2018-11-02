#include "platform.h"

#include "lua.h"
#include "lua_functions.h"

#include "console/console.h"
#include "vr/vr.h"
#include "signatures/signatures.h"
#include "tweaker/xmltweaker.h"

#include "subhook.h"

#include <fstream>
#include <string>
#include <thread>

using namespace std;
using namespace pd2hook;

static CConsole* console = NULL;

static bool vrMode;
static std::thread::id main_thread_id;

blt::idstring *blt::platform::last_loaded_name = idstring_none, *blt::platform::last_loaded_ext = idstring_none;

static subhook::Hook gameUpdateDetour, newStateDetour, newStateDetourVr, luaCloseDetour, node_from_xmlDetour;

static void init_idstring_pointers()
{
	char *tmp;

	if (try_open_base_vr)
	{
		tmp = (char*)try_open_base_vr;
		tmp += 0x44;
		blt::platform::last_loaded_name = *((blt::idstring**)tmp);

		tmp = (char*)try_open_base_vr;
		tmp += 0x2A;
		blt::platform::last_loaded_ext = *((blt::idstring**)tmp);
	}
	else
	{
		tmp = (char*)try_open_base;
		tmp += 0x17;
		blt::platform::last_loaded_name = *((blt::idstring**)tmp);

		tmp = (char*)try_open_base;
		tmp += 0x05;
		blt::platform::last_loaded_ext = *((blt::idstring**)tmp);
	}
}

static int __fastcall luaL_newstate_new(void* thislol, int edx, char no, char freakin, int clue)
{
	subhook::ScopedHookRemove scoped_remove(vrMode ? &newStateDetourVr : &newStateDetour);

	int ret = (vrMode ? luaL_newstate_vr : luaL_newstate)(thislol, no, freakin, clue);

	lua_State* L = (lua_State*)*((void**)thislol);
	printf("Lua State: %p\n", (void*)L);
	if (!L) return ret;

	blt::lua_functions::initiate_lua(L);

	return ret;
}

static int __fastcall luaL_newstate_new_vr(void* thislol, int edx, char no, char freakin, int clue)
{
	vrMode = true;
	return luaL_newstate_new(thislol, edx, no, freakin, clue);
}

void* __fastcall do_game_update_new(void* thislol, int edx, int* a, int* b)
{
	subhook::ScopedHookRemove scoped_remove(&gameUpdateDetour);

	// If someone has a better way of doing this, I'd like to know about it.
	// I could save the this pointer?
	// I'll check if it's even different at all later.
	if (std::this_thread::get_id() != main_thread_id)
	{
		return do_game_update(thislol, a, b);
	}

	lua_State* L = (lua_State*)*((void**)thislol);

	blt::lua_functions::update(L);

	return do_game_update(thislol, a, b);
}

void lua_close_new(lua_State* L)
{
	subhook::ScopedHookRemove scoped_remove(&luaCloseDetour);

	blt::lua_functions::close(L);
	lua_close(L);
}


#include "tweaker/xmltweaker_internal.h"
void __stdcall edit_node_from_xml_hook(int arg);
static void __declspec(naked) node_from_xml_new()
{
	__asm
	{
		push ecx

		// Push on, as the last argument, the value of a pointer passed as the last function argument
		// This appears to be a length value, however it will often have a huge (and fixed) value.
		push[esp + 12]

		push edx

		// Run the intercept method
		call pd2hook::tweaker::tweak_pd2_xml

		pop edx

		add esp, 4 // Remove the last argument

		pop ecx

		// Move across our string
		mov edx, eax

		// Save our value for later, so we can free it
		push eax

		// Bring the old top-of-the-stack back up
		mov eax, [esp + 4]
		push eax

		// Disarm the hook
		// (wouldn't it be great if subhook had reliable trampolines? Sometime I should add and PR that)
		push eax
		push ecx
		push edx
		push 0
		call edit_node_from_xml_hook
		pop edx
		pop ecx
		pop eax

		// Call the original function
		call node_from_xml

		// Rearm the hook
		push eax
		push ecx
		push edx
		push 1
		call edit_node_from_xml_hook
		pop edx
		pop ecx
		pop eax

		add esp, 4 // Remove our temporary value from the stack

		// Currently, the stack is: [ arg3, txt
		// Call the free function, which uses the txt arg
		call pd2hook::tweaker::free_tweaked_pd2_xml

		// Clear off the txt argument, and the stack is back to how we left it
		add esp, 4

		retn
	}
}

void __stdcall edit_node_from_xml_hook(int arg)
{
	if (arg)
	{
		node_from_xmlDetour.Install(node_from_xml, node_from_xml_new);
	}
	else
	{
		node_from_xmlDetour.Remove();
	}
}


void blt::platform::InitPlatform()
{
	main_thread_id = std::this_thread::get_id();

	// Set up logging first, so we can see messages from the signature search process
#ifdef INJECTABLE_BLT
	gbl_mConsole = new CConsole();
#else
	ifstream infile("mods/developer.txt");
	string debug_mode;
	if (infile.good())
	{
		debug_mode = "post"; // default value
		infile >> debug_mode;
	}
	else
	{
		debug_mode = "disabled";
	}

	if (debug_mode != "disabled")
		console = new CConsole();
#endif

	SignatureSearch::Search();

	if (node_from_xml == NULL) node_from_xml = node_from_xml_vr;

	gameUpdateDetour.Install(do_game_update, do_game_update_new);
	if(luaL_newstate)
		newStateDetour.Install(luaL_newstate, luaL_newstate_new);
	else
		newStateDetourVr.Install(luaL_newstate_vr, luaL_newstate_new_vr);
	luaCloseDetour.Install(lua_close, lua_close_new);

	edit_node_from_xml_hook(true);

	VRManager::CheckAndLoad();

	init_idstring_pointers();
}

void blt::platform::ClosePlatform()
{
	// Okay... let's not do that.
	// I don't want to keep this in memory, but it CRASHES THE SHIT OUT if you delete this after all is said and done.
	if (console) delete console;
}

void blt::platform::GetPlatformInformation(lua_State * L)
{
	lua_pushstring(L, "mswindows");
	lua_setfield(L, -2, "platform");

	lua_pushstring(L, "arch");
	lua_setfield(L, -2, "x86");
}

void blt::platform::win32::OpenConsole()
{
	if (!console)
	{
		console = new CConsole();
	}
}

void * blt::platform::win32::get_lua_func(const char* name)
{
	// Only allow getting the Lua functions
	if (strncmp(name, "lua", 3)) return NULL;

	// Don't allow getting the setup functions
	if (!strncmp(name, "luaL_newstate", 13)) return NULL;

	return SignatureSearch::GetFunctionByName(name);
}

subhook::Hook luaCallDetour;

bool blt::platform::lua::GetForcePCalls()
{
	return luaCallDetour.IsInstalled();
}

void blt::platform::lua::SetForcePCalls(bool state)
{
	// Don't change if already set up
	if (state == GetForcePCalls()) return;

	if (state)
	{
		luaCallDetour.Install(lua_call, blt::lua_functions::perform_lua_pcall);
		//PD2HOOK_LOG_LOG("blt.forcepcalls(): Protected calls will now be forced");
	}
	else
	{
		luaCallDetour.Remove();
		//PD2HOOK_LOG_LOG("blt.forcepcalls(): Protected calls are no longer being forced");
	}
}
