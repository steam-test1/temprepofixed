#include "platform.h"
#include "lua_functions.h"

#include "console/console.h"
#include "vr/vr.h"
#include "signatures/signatures.h"
#include "lua.h"

#include "tweaker/xmltweaker.h"

#include <fstream>
#include <string>
#include <thread>

using namespace std;
using namespace pd2hook;

static CConsole* console = NULL;

static bool vrMode;
static std::thread::id main_thread_id;

static int __fastcall luaL_newstate_new(void* thislol, int edx, char no, char freakin, int clue)
{
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

	// If someone has a better way of doing this, I'd like to know about it.
	// I could save the this pointer?
	// I'll check if it's even different at all later.
	if (std::this_thread::get_id() != main_thread_id) {
		return do_game_update(thislol, a, b);
	}

	lua_State* L = (lua_State*)*((void**)thislol);

	blt::lua_functions::update(L);

	return do_game_update(thislol, a, b);
}

void lua_close_new(lua_State* L) {
	blt::lua_functions::close(L);
	lua_close(L);
}

void blt::platform::InitPlatform() {
	main_thread_id = std::this_thread::get_id();

	// Set up logging first, so we can see messages from the signature search process
#ifdef INJECTABLE_BLT
	gbl_mConsole = new CConsole();
#else
	ifstream infile("mods/developer.txt");
	string debug_mode;
	if (infile.good()) {
		debug_mode = "post"; // default value
		infile >> debug_mode;
	}
	else {
		debug_mode = "disabled";
	}

	if (debug_mode != "disabled")
		console = new CConsole();
#endif

	SignatureSearch::Search();

	if (node_from_xml == NULL) node_from_xml = node_from_xml_vr;

	FuncDetour* gameUpdateDetour = new FuncDetour((void**)&do_game_update, do_game_update_new);
	FuncDetour* newStateDetour = new FuncDetour((void**)&luaL_newstate, luaL_newstate_new);
	FuncDetour* newStateDetourVr = new FuncDetour((void**)&luaL_newstate_vr, luaL_newstate_new_vr);
	FuncDetour* luaCloseDetour = new FuncDetour((void**)&lua_close, lua_close_new);

	FuncDetour* node_from_xmlDetour = new FuncDetour((void**)&node_from_xml, tweaker::node_from_xml_new);

	VRManager::CheckAndLoad();
}

void blt::platform::ClosePlatform() {
	// Okay... let's not do that.
	// I don't want to keep this in memory, but it CRASHES THE SHIT OUT if you delete this after all is said and done.
	if (console) delete console;
}

void blt::platform::win32::OpenConsole() {
	if (!console) {
		console = new CConsole();
	}
}
