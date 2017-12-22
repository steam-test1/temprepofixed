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

idstring *tweaker::last_loaded_name = NULL, *tweaker::last_loaded_ext = NULL;

void tweaker::init_xml_tweaker() {
	char *tmp;

	if (try_open_base_vr) {
		tmp = (char*)try_open_base_vr;
		tmp += 0x3D;
		last_loaded_name = *((idstring**)tmp);

		tmp = (char*)try_open_base_vr;
		tmp += 0x23;
		last_loaded_ext = *((idstring**)tmp);
	}
	else {
		tmp = (char*)try_open_base;
		tmp += 0x26;
		last_loaded_name = *((idstring**)tmp);

		tmp = (char*)try_open_base;
		tmp += 0x14;
		last_loaded_ext = *((idstring**)tmp);
	}

	printf("name: %p, ext: %p\n", last_loaded_name, last_loaded_ext);
}

void* __cdecl tweaker::tweak_pd2_xml(char* text) {
	const char* new_text = transform_file(text);
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

	return (char*)buffer;
}

void __cdecl tweaker::free_tweaked_pd2_xml(char* text) {
	if (buffers.erase(text)) {
		free(text);
	}
}
