#include <fstream>
#include <vector>

#include "global.h"
#include "xmltweaker_internal.h"
#include "util/util.h"
#include "wrenxml.h"

extern "C" {
#include "wren.h"
}

using namespace pd2hook;
using namespace pd2hook::tweaker;
using namespace std;

static WrenVM *vm = NULL;

static void err(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message)
{
	if (module == NULL) module = "<unknown>";
	PD2HOOK_LOG_LOG(string("[WREN ERR] ") + string(module) + ":" + to_string(line) + " ] " + message);
}

static void log(WrenVM* vm)
{
	const char *text = wrenGetSlotString(vm, 1);
	PD2HOOK_LOG_LOG(string("[WREN] ") + text);
}

static string file_to_string(ifstream &in) {
	return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

void io_listDirectory(WrenVM* vm) {
	string filename = wrenGetSlotString(vm, 1);
	bool dir = wrenGetSlotBool(vm, 2);
	vector<string> files = Util::GetDirectoryContents(filename, dir);

	wrenSetSlotNewList(vm, 0);

	for (string const &file : files)
	{
		if (file == "." || file == "..") continue;

		wrenSetSlotString(vm, 1, file.c_str());
		wrenInsertInList(vm, 0, -1, 1);
	}
}

void io_info(WrenVM* vm) {
	const char* path = wrenGetSlotString(vm, 1);

	Util::FileType type = Util::GetFileType(path);

	if (type == Util::FileType_None) {
		wrenSetSlotString(vm, 0, "none");
	}
	else if (type == Util::FileType_Directory) {
		wrenSetSlotString(vm, 0, "dir");
	}
	else {
		wrenSetSlotString(vm, 0, "file");
	}
}

void io_read(WrenVM *vm) {
	string file = wrenGetSlotString(vm, 1);

	ifstream handle(file);
	if (!handle.good()) {
		PD2HOOK_LOG_ERROR("Wren IO.read: Could not load file " + file);
		exit(1);
	}

	string contents = file_to_string(handle);
	wrenSetSlotString(vm, 0, contents.c_str());
}

void io_dynamic_import(WrenVM *vm) {
	// TODO do this properly
	string module = wrenGetSlotString(vm, 1);

	string line = string("import \"") + module + string("\"");
	WrenInterpretResult compileResult = wrenInterpret(vm, line.c_str());
	printf("Module Load: %d\n", compileResult);
}

void io_idstring_hash(WrenVM *vm) {
	blt::idstring hash = idstring_hash(wrenGetSlotString(vm, 1));

	char hex[17]; // 16-chars long +1 for the null
	snprintf(hex, sizeof(hex), "%016llx", hash);
	wrenSetSlotString(vm, 0, hex);
}

static WrenForeignClassMethods bindForeignClass(
	WrenVM* vm, const char* module, const char* class_name) {
	WrenForeignClassMethods methods = wrenxml::get_XML_class_def(vm, module, class_name);

	return methods;
}

static WrenForeignMethodFn bindForeignMethod(
	WrenVM* vm,
	const char* module,
	const char* className,
	bool isStatic,
	const char* signature)
{
	WrenForeignMethodFn wxml_method = wrenxml::bind_wxml_method(vm, module, className, isStatic, signature);
	if (wxml_method) return wxml_method;

	if (strcmp(module, "base/native") == 0)
	{
		if (strcmp(className, "Logger") == 0)
		{
			if (isStatic && strcmp(signature, "log(_)") == 0)
			{
				return &log; // C function for Math.add(_,_).
			}
			// Other foreign methods on Math...
		}
		else if (strcmp(className, "IO") == 0)
		{
			if (isStatic && strcmp(signature, "listDirectory(_,_)") == 0)
			{
				return &io_listDirectory;
			}
			if (isStatic && strcmp(signature, "info(_)") == 0)
			{
				return &io_info;
			}
			if (isStatic && strcmp(signature, "read(_)") == 0)
			{
				return &io_read;
			}
			if (isStatic && strcmp(signature, "dynamic_import(_)") == 0)
			{
				return &io_dynamic_import;
			}
			if (isStatic && strcmp(signature, "idstring_hash(_)") == 0)
			{
				return &io_idstring_hash;
			}
		}
		// Other classes in main...
	}
	// Other modules...

	return NULL;
}

static char* getModulePath(WrenVM* vm, const char* name_c)
{
	string name = name_c;
	string mod = name.substr(0, name.find_first_of('/'));
	string file = name.substr(name.find_first_of('/') + 1);

	ifstream handle("mods/" + mod + "/wren/" + file + ".wren");
	if (!handle.good()) {
		return NULL;
	}

	string str = file_to_string(handle);

	size_t length = str.length() + 1;
	char* output = (char*)malloc(length); // +1 for the null
	portable_strncpy(output, str.c_str(), length);

	return output; // free()d by Wren
}

static bool available = true;

const char* tweaker::transform_file(const char* text)
{
	if (vm == NULL) {
		if (available) {
			// If the main file doesn't exist, do nothing
			Util::FileType ftyp = Util::GetFileType("mods/base/wren/base.wren");
			if (ftyp == Util::FileType_None) available = false;
		}

		if (!available) return text;

		WrenConfiguration config;
		wrenInitConfiguration(&config);
		config.errorFn = &err;
		config.bindForeignMethodFn = &bindForeignMethod;
		config.bindForeignClassFn = &bindForeignClass;
		config.loadModuleFn = &getModulePath;
		vm = wrenNewVM(&config);

		WrenInterpretResult compileResult = wrenInterpret(vm, R"!( import "base/base" )!");
		printf("Compile: %d\n", compileResult);
	}

	wrenEnsureSlots(vm, 4);

	wrenGetVariable(vm, "base/base", "BaseTweaker", 0);
	WrenHandle* tweakerClass = wrenGetSlotHandle(vm, 0);
	WrenHandle* sig = wrenMakeCallHandle(vm, "tweak(_,_,_)");

	char hex[17]; // 16-chars long +1 for the null

	wrenSetSlotHandle(vm, 0, tweakerClass);

	snprintf(hex, sizeof(hex), "%016llx", *blt::platform::last_loaded_name);
	wrenSetSlotString(vm, 1, hex);

	snprintf(hex, sizeof(hex), "%016llx", *blt::platform::last_loaded_ext);
	wrenSetSlotString(vm, 2, hex);

	wrenSetSlotString(vm, 3, text);

	WrenInterpretResult result2 = wrenCall(vm, sig);

	wrenReleaseHandle(vm, tweakerClass);
	wrenReleaseHandle(vm, sig);

	const char* new_text = wrenGetSlotString(vm, 0);

	return new_text;
}
