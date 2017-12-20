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

static unordered_set<char*> buffers;

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

	const char* new_text = tweaker::transform_file(text);
	size_t length = strlen(new_text) + 1; // +1 for the null

	char* buffer = (char*)malloc(length);
	buffers.insert(buffer);

	strcpy_s(buffer, length, new_text);

	//if (!strncmp(new_text, "<network>", 9)) {
	//	std::ofstream out("output.txt");
	//	out << new_text;
	//	out.close();
	//}

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
