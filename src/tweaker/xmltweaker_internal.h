#pragma once

#include "xmltweaker.h"

namespace pd2hook {
	namespace tweaker {
		typedef unsigned long long idstring;

		void* __cdecl tweak_pd2_xml(char* text);
		void __cdecl free_tweaked_pd2_xml(char* text);

		//void note_loaded_file_wrapper(idstring ext, idstring name);
	};
};
