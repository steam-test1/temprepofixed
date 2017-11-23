#include "vr/vr.h"
#include "vr/openvr.h"
#include "util/util.h"
#include "windows.h"
/*
using namespace vr;

typedef void* (*VR_Init_t)(EVRInitError *peError, EVRApplicationType eApplicationType, const char *pStartupInfo);

void *VR_CALLTYPE VR_GetGenericInterface_hook(const char *pchInterfaceVersion, EVRInitError *peError) {
	MessageBox(0, "hi", "hi", MB_OK);
}

namespace pd2hook
{

VRManager::VRManager(){
	PD2HOOK_LOG_LOG("Hooking SteamVR");

	VR_Init_t func = (VR_Init_t) GetProcAddress(GetModuleHandle("openvr_api.dll"), "VR_GetGenericInterface");

	TCHAR msg[100];
	snprintf(msg, 100, "func: %p", func);
	MessageBox(0, msg, "prehook", MB_OK);
}

VRManager::~VRManager(){
}

VRManager* VRManager::GetInstance() {
	// Just keep an instance open
	static VRManager vrSingleton;
	return &vrSingleton;
}

}
*/