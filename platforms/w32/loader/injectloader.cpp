// For the injectable BLT, we don't need to do anything except for loading the PD2 hook
#ifdef INJECTABLE_BLT

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include "InitState.h"

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		pd2hook::InitiateStates();
	}

	return 1;
}

#endif
