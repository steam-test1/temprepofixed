
#include "XAudio.h"

#ifdef ENABLE_XAUDIO

#include "XAudioInternal.h"
#include "stb_vorbis.h"

namespace pd2hook {
	using namespace pd2hook::xaudio;

	int xasource::lX_new_source(lua_State *L) {
		int count = lua_gettop(L) == 0 ? 1 : lua_tointeger(L, 1);
		lua_settop(L, 0);

		ALuint sources[32];

		if (count > 32) {
			PD2HOOK_LOG_LOG("Attempted to create more than 32 ALsources in a single call!");
			return 0;
		}

		// Generate Sources 
		alGenSources(count, sources);

		// TODO expand stack to ensure we can't crash

		// Error reporting
		ALenum error;
		if ((error = alGetError()) != AL_NO_ERROR) {
			throw "alGenSources 1 : " + error;
		}

		for (size_t i = 0; i < count; i++) {
			XASource *buff = new XASource(sources[i]);
			*(XALuaHandle*)lua_newuserdata(L, sizeof(XALuaHandle)) = XALuaHandle(buff);

			// Set the metatable
			luaL_getmetatable(L, "XAudio.source");
			lua_setmetatable(L, -2);
		}

		return count;
	}

	int xasource::XASource_close(lua_State *L) {
		// TODO check userdata

		((XALuaHandle*)lua_touserdata(L, 1))->Close(true);

		// TODO remove from openSources when applicable

		return 0;
	}

	int xasource::XASource_set_buffer(lua_State *L) {
		XALuaHandle *xthis = (XALuaHandle*)lua_touserdata(L, 1);
		// TODO validate 'valid' flag

		bool nil = lua_isnil(L, 2);
		ALuint buffid;
		if (nil) {
			buffid = 0; // NULL Buffer - basically, no audio
		}
		else {
			XALuaHandle *buff = (XALuaHandle*)lua_touserdata(L, 2);
			// TODO validate 'valid' flag
			buffid = buff->Handle(L);
		}

		// Attach buffer 0 to source 
		alSourcei(xthis->Handle(L), AL_BUFFER, buffid);
		ALenum error;
		if ((error = alGetError()) != AL_NO_ERROR) {
			throw string("alSourcei AL_BUFFER 0 : " + error);
		}

		return 0;
	}

	int xasource::XASource_play(lua_State *L) {
		XALuaHandle *xthis = (XALuaHandle*)lua_touserdata(L, 1);
		// TODO validate 'valid' flag

		alSourcePlay(xthis->Handle(L));
		// TODO error checking

		return 0;
	}

	static void set_vector_property(lua_State *L, ALenum type) {
		XALuaHandle *xthis = (XALuaHandle*)lua_touserdata(L, 1);
		// TODO validate 'valid' flag

		alSource3f(
			xthis->Handle(L),
			type,
			WORLD_VEC(L, 2, 3, 4)
		);
	}

	int xasource::XASource_set_position(lua_State *L) {
		set_vector_property(L, AL_POSITION);
		return 0;
	}

	int xasource::XASource_set_velocity(lua_State *L) {
		set_vector_property(L, AL_VELOCITY);
		return 0;
	}

	int xasource::XASource_set_direction(lua_State *L) {
		// FIXME doesn't *seem* to work?
		set_vector_property(L, AL_DIRECTION);
		return 0;
	}
};

#endif