#ifndef __VR_HEADER__
#define __VR_HEADER__

namespace pd2hook
{
class VRManager {
private:
	VRManager();

public:
	~VRManager();

	static VRManager* VRManager::GetInstance();
private:
};
}

#endif // __VR_HEADER__