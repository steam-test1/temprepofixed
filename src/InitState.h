#ifndef __INIT_STATE__
#define __INIT_STATE__

namespace pd2hook
{
	void InitiateStates();
	void DestroyStates();

	bool check_active_state(void* L);
}

#endif