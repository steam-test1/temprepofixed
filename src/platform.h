#pragma once

namespace blt {
	#define idstring_none 0
	typedef unsigned long long idstring;

	namespace platform {
		extern idstring *last_loaded_name, *last_loaded_ext;

		void InitPlatform();
		void ClosePlatform();

#ifdef _WIN32
		namespace win32 {
			void OpenConsole();
		};
#endif
	};
};
