#pragma once

namespace blt {
	namespace platform {
		void PreInitPlatform();
		void InitPlatform();
		void ClosePlatform();

#ifdef _WIN32
		namespace win32 {
			void OpenConsole();
		};
#endif
	};
};
