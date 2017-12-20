#include <fstream>
#include <vector>

#include "xmltweaker_internal.h"
#include "util/util.h"

extern "C" {
#include "wren.h"
}

using namespace pd2hook;
using namespace pd2hook::tweaker;
using namespace std;

static WrenVM *vm = NULL;

static void err(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message)
{
	PD2HOOK_LOG_LOG(string("[WREN ERR] ") + string(module) + ":" + to_string(line) + " ] " + message);
}

static void log(WrenVM* vm)
{
	const char *text = wrenGetSlotString(vm, 1);
	PD2HOOK_LOG_LOG(string("[WREN] ") + text);
}

static WrenForeignMethodFn bindForeignMethod(
	WrenVM* vm,
	const char* module,
	const char* className,
	bool isStatic,
	const char* signature)
{
	if (strcmp(module, "base/base") == 0)
	{
		if (strcmp(className, "Logger") == 0)
		{
			if (isStatic && strcmp(signature, "log(_)") == 0)
			{
				return &log; // C function for Math.add(_,_).
			}
			// Other foreign methods on Math...
		}
		// Other classes in main...
	}
	// Other modules...

	return NULL;
}

static char* getModulePath(WrenVM* vm, const char* name)
{
	string xname = string("mods/") + string(name) + string(".wren");

	ifstream ifs(xname);
	if (!ifs.good()) {
		return NULL;
	}
	string str((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());

	size_t length = str.length() + 1;
	char* output = (char*)malloc(length); // +1 for the null
	strcpy_s(output, length, str.c_str());

	return output; // free()d by Wren
}

const char* tweaker::transform_file(const char* text)
{
	if (vm == NULL) {
		WrenConfiguration config;
		wrenInitConfiguration(&config);
		config.errorFn = &err;
		config.bindForeignMethodFn = &bindForeignMethod;
		config.loadModuleFn = &getModulePath;
		vm = wrenNewVM(&config);

		WrenInterpretResult compileResult = wrenInterpret(vm, R"!( import "base/base" )!");
		printf("Compile: %d\n", compileResult);
		if (compileResult != WREN_RESULT_SUCCESS) Sleep(20000);
	}

	wrenEnsureSlots(vm, 4);

	wrenGetVariable(vm, "base/base", "BaseTweaker", 0);
	WrenHandle* tweakerClass = wrenGetSlotHandle(vm, 0);
	WrenHandle* sig = wrenMakeCallHandle(vm, "tweak(_,_,_)");

	char hex[17]; // 16-chars long +1 for the null

	wrenSetSlotHandle(vm, 0, tweakerClass);

	sprintf_s(hex, 17, "%016llx", tweaker::last_loaded_name);
	wrenSetSlotString(vm, 1, hex);

	sprintf_s(hex, 17, "%016llx", tweaker::last_loaded_ext);
	wrenSetSlotString(vm, 2, hex);

	wrenSetSlotString(vm, 3, text);

	WrenInterpretResult result2 = wrenCall(vm, sig);

	wrenReleaseHandle(vm, tweakerClass);
	wrenReleaseHandle(vm, sig);

	const char* new_text = wrenGetSlotString(vm, 0);

	return new_text;
}
