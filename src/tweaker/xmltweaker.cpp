#include "xmltweaker_internal.h"
#include <stdio.h>
#include <fstream>
#include <unordered_set>
#include "util/util.h"
#include "signatures/sigdef.h"

extern "C" {
#include "wren.h"
}

using namespace std;
using namespace pd2hook;
using namespace tweaker;

static WrenVM *vm = NULL;
static unordered_set<char*> buffers;

static void err(WrenVM* vm, WrenErrorType type, const char* module, int line,
	const char* message) {
	PD2HOOK_LOG_LOG(std::string("[WREN ERR] ") + message);
}

void log(WrenVM* vm)
{
	const char *text = wrenGetSlotString(vm, 1);
	PD2HOOK_LOG_LOG(std::string("[WREN] ") + text);
}

WrenForeignMethodFn bindForeignMethod(
	WrenVM* vm,
	const char* module,
	const char* className,
	bool isStatic,
	const char* signature)
{
	if (strcmp(module, "main") == 0)
	{
		if (strcmp(className, "BaseTweaker") == 0)
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
}

idstring tweaker::last_loaded_name = NULL, tweaker::last_loaded_ext = NULL;
void tweaker::note_loaded_file(idstring name, idstring ext) {
	tweaker::last_loaded_name = name;
	tweaker::last_loaded_ext = ext;
}

void* __cdecl tweaker::tweak_pd2_xml(char* text) {
	// TODO better way to see if this isn't the actual file's name
	if (tweaker::last_loaded_name == NULL) {
		// PD2HOOK_LOG_LOG("Name unknown for parsed XML function")
		return text;
	}

	if (vm == NULL) {
		WrenConfiguration config;
		wrenInitConfiguration(&config);
		config.errorFn = &err;
		config.bindForeignMethodFn = &bindForeignMethod;
		vm = wrenNewVM(&config);

		WrenInterpretResult compileResult = wrenInterpret(vm, R"!(
class BaseTweaker {
	foreign static log(text)
	static tweak(name, ext, text) {
		log("Loading %(name).%(ext)")
		if(text.startsWith("<network>") && text.contains("sync_cs_grenade")) {
			var from_txt = "\t\t\t<param type=\"int\" min=\"0\" max=\"600\"/>\r\n\t\t</message>"
			var to_txt = "<param type=\"int\" min=\"0\" max=\"600\"/><param type=\"int\" min=\"0\" max=\"600\"/><param type=\"int\" min=\"0\" max=\"600\"/></message>"
			log(from_txt)
			text = text.replace(from_txt, to_txt)
			BaseTweaker.log("Modified XML : %(text.indexOf(to_txt)), %(text.indexOf(to_txt, text.indexOf(to_txt) + 1))")
		}
		return text
	}
}
)!");
		printf("Compile: %d\n", compileResult);
		if (compileResult != WREN_RESULT_SUCCESS) Sleep(20000);
	}

	wrenEnsureSlots(vm, 4);

	wrenGetVariable(vm, "main", "BaseTweaker", 0);
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

	const char* new_text = wrenGetSlotString(vm, 0);
	size_t length = strlen(new_text) + 1; // +1 for the null

	char* buffer = (char*)malloc(length);
	buffers.insert(buffer);

	strcpy_s(buffer, length, new_text);

	//if (!strncmp(new_text, "<network>", 9)) {
	//	std::ofstream out("output.txt");
	//	out << new_text;
	//	out.close();
	//}

	wrenReleaseHandle(vm, tweakerClass);
	wrenReleaseHandle(vm, sig);

	/*static int counter = 0;
	if (counter++ == 1) {
		printf("%s\n", buffer);

		const int kMaxCallers = 62;
		void* callers[kMaxCallers];
		int count = CaptureStackBackTrace(0, kMaxCallers, callers, NULL);
		for (int i = 0; i < count; i++)
			printf("*** %d called from .text:%08X\n", i, callers[i]);
		Sleep(20000);
	}*/

	// Reset the name so we don't think we're parsing the file again
	tweaker::last_loaded_name = NULL;
	tweaker::last_loaded_ext = NULL;

	return (char*)buffer;
}

void __cdecl tweaker::free_tweaked_pd2_xml(char* text) {
	if (buffers.erase(text)) {
		free(text);
	}
}
