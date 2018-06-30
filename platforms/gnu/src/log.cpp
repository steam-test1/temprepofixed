#include "blt/log.hh"
#include "util/util.h"

namespace blt
{
	namespace log
	{

		using std::string;
		using blt::log::MessageType;

		void
		log(string msg, MessageType mt)
		{
			switch(mt)
			{
			case LOG_INFO:
				PD2HOOK_LOG_LOG(msg);
				break;
			case LOG_LUA:
				PD2HOOK_LOG_LUA(msg);
				break;
			case LOG_WARN:
				PD2HOOK_LOG_WARN(msg);
				break;
			case LOG_ERROR:
				PD2HOOK_LOG_ERROR(msg);
				break;
			}
		}

		void
		finalize()
		{
			// TODO implement
		}
	}
}

/* vim: set ts=4 softtabstop=0 sw=4 expandtab: */

