#include "vr/vr.h"
#include "vr/openvr.h"
#include "util/util.h"
#include "signatures/signatures.h"
#include "windows.h"

using namespace vr;

namespace pd2hook {

	typedef void* (__cdecl *VR_GetGenericInterface_t)(const char *pchInterfaceVersion, EVRInitError *peError);

	VR_GetGenericInterface_t func;
	IVRSystem *steamvr = 0;
	void *VR_CALLTYPE __cdecl VR_GetGenericInterface_hook(const char *pchInterfaceVersion, EVRInitError *peError) {
		void* res = func(pchInterfaceVersion, peError);

		if (!strcmp(IVRSystem_Version, pchInterfaceVersion)) {
			steamvr = (IVRSystem*)res;
		}

		return res;
	}

	VRManager::VRManager() {
		PD2HOOK_LOG_LOG("Hooking SteamVR");

		HMODULE api = GetModuleHandle("openvr_api.dll");
		func = (VR_GetGenericInterface_t)GetProcAddress(api, "VR_GetGenericInterface");

		FuncDetour* gameUpdateDetour = new FuncDetour((void**)&func, VR_GetGenericInterface_hook);
	}

	VRManager::~VRManager() {
	}

	bool VRManager::IsLoaded() {
		return steamvr;
	}

	std::string VRManager::GetHMDBrand() {
		if (steamvr == NULL) return "";

		char name[100];
		uint32_t len = steamvr->GetStringTrackedDeviceProperty(
			k_unTrackedDeviceIndex_Hmd,
			Prop_ManufacturerName_String,
			name, 100
		);

		if (len == 0) return "";

		return std::string(name, len - 1); // SteamVR includes null, std::string doesn't.
	}

	bool VRManager::IsExtraButtonPressed(int hand) {
		VRControllerState_t state;
		ETrackedControllerRole role = hand == 1 ? TrackedControllerRole_LeftHand : TrackedControllerRole_RightHand;
		int id = steamvr->GetTrackedDeviceIndexForControllerRole(role);
		steamvr->GetControllerState(id, &state, sizeof(VRControllerState_t));

		uint64_t mask = ButtonMaskFromId(k_EButton_A);

		return state.ulButtonPressed && mask;
	}

	VRManager* VRManager::GetInstance() {
		// Just keep an instance open
		static VRManager vrSingleton;
		return &vrSingleton;
	}

	void VRManager::CheckAndLoad() {
		if (GetModuleHandle("openvr_api.dll"))
			GetInstance();
	}

}
