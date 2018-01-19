#pragma once

#include "platform.h"
#include "xmltweaker.h"
#include <string>

namespace pd2hook {
	namespace tweaker {
		void* tweak_pd2_xml(char* text, int32_t text_length);
		void free_tweaked_pd2_xml(char* text);

		/**
		* Transforms the contents of the file
		* the return result MUST be valid immediately after the call returns,
		* but not necessaraly after that.
		*/
		const char* transform_file(const char* contents);

		blt::idstring idstring_hash(std::string text);
	};
};
