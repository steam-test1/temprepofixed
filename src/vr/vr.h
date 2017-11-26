#ifndef __VR_HEADER__
#define __VR_HEADER__

#include <string>

namespace pd2hook
{
class VRManager {
private:
	VRManager();

public:
	~VRManager();

	bool IsLoaded();

	std::string GetHMDBrand();

	bool IsExtraButtonPressed(int id);

	static VRManager* VRManager::GetInstance();

	static void CheckAndLoad();
private:
};
}

#endif // __VR_HEADER__

