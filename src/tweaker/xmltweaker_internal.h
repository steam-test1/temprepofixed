#pragma once

#include "xmltweaker.h"

namespace pd2hook {
	namespace tweaker {
		typedef unsigned long long idstring;

		extern idstring last_loaded_name, last_loaded_ext;

		void* __cdecl tweak_pd2_xml(char* text);
		void __cdecl free_tweaked_pd2_xml(char* text);

		void note_loaded_file(idstring name, idstring ext);
	};
};
