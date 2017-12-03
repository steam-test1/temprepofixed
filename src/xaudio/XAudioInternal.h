#pragma once

#include "XAudio.h"

#ifdef ENABLE_XAUDIO

#include <string>
#include <map>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

#include "signatures/sigdef.h"
#include "util/util.h"

using namespace std;

#define WORLD_VEC_CUSTOMSCALE(scale, L, ix, iy, iz) \
(lua_tonumber(L, ix) / scale), \
(lua_tonumber(L, iy) / scale), \
(lua_tonumber(L, iz) / scale)
#define WORLD_VEC(L, ix, iy, iz) WORLD_VEC_CUSTOMSCALE(xaudio::world_scale, L, ix, iy, iz)

namespace pd2hook {
	namespace xasource {
		class XASource;
	};
	namespace xabuffer {
		class XABuffer;
	};

	namespace xaudio {
		class XAResource {
		public:
			XAResource(ALuint alhandle) : alhandle(alhandle) {}
			ALuint Handle() {
				return alhandle;
			}
			void Employ() {
				usecount++;
			}
			void Discard(bool force);
			void Close();
		private:
			const ALuint alhandle;
			bool valid = true;
			int usecount = 0;
		};

		class XALuaHandle {
		public:
			XALuaHandle(XAResource *resource);
			ALuint Handle(lua_State *L);
			void Close(bool force);
			bool Ready() {
				return open;
			}
		private:
			XAResource *resource;
			bool open = true;
		};

		extern map<string, xabuffer::XABuffer*> openBuffers;
		extern vector<xasource::XASource*> openSources;
		extern double world_scale;
	};

	namespace xabuffer {
		class XABuffer : public xaudio::XAResource {
			using xaudio::XAResource::XAResource;
		};

		int lX_loadbuffer(lua_State *L);
		int XABuffer_close(lua_State *L);
	};

	namespace xasource {
		class XASource : public xaudio::XAResource {
			using xaudio::XAResource::XAResource;
		};

		int lX_new_source(lua_State *L);
		int XASource_close(lua_State *L);
		int XASource_set_buffer(lua_State *L);
		int XASource_play(lua_State *L);
		int XASource_pause(lua_State *L);
		int XASource_stop(lua_State *L);
		int XASource_get_state(lua_State *L);

		int XASource_set_position(lua_State *L);
		int XASource_set_velocity(lua_State *L);
		int XASource_set_direction(lua_State *L);
	};

	namespace xalistener {
		int XAListener_set_velocity(lua_State *L);
		int XAListener_set_position(lua_State *L);
		int XAListener_set_orientation(lua_State *L);
		void lua_register(lua_State *L);
	};
};

#endif
