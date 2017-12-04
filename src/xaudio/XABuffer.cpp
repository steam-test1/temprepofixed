
#include "XAudio.h"

#ifdef ENABLE_XAUDIO

#include "XAudioInternal.h"
#include "stb_vorbis.h"

namespace pd2hook {
	using namespace xaudio;

	int xabuffer::lX_loadbuffer(lua_State *L) {
		ALERR;

		int count = lua_gettop(L);

		ALuint buffers[32];

		if (count > 32) {
			XAERR("Attempted to create more than 32 ALbuffers in a single call!");
		}

		vector<string> filenames;
		for (size_t i = 0; i < count; i++) {
			// i+1 because the Lua stack starts at 1, not 0
			filenames.push_back(lua_tostring(L, i + 1));
		}
		lua_settop(L, 0);

		alGenBuffers(count, buffers);
		ALERR;

		for (size_t i = 0; i < count; i++) {
			string filename = filenames[i];

			if (openBuffers.count(filename)) {
				*(XALuaHandle*)lua_newuserdata(L, sizeof(XALuaHandle)) = XALuaHandle(openBuffers[filename]);
				// TODO don't create buffers for cached stuff
			}
			else {
				XABuffer *buff = new XABuffer(buffers[i]);
				*(XALuaHandle*)lua_newuserdata(L, sizeof(XALuaHandle)) = XALuaHandle(buff);

				openBuffers[filename] = buff;

				// Set the metatable
				luaL_getmetatable(L, "XAudio.buffer");
				lua_setmetatable(L, -2);

				// Load the contents of the buffer

				int vorbisLen, channels, sampleRate;
				short *data;

				vorbisLen = stb_vorbis_decode_filename(filename.c_str(),
					&channels,
					&sampleRate,
					&data);

				// Copy the file into our buffer
				// TODO do this in the background
				ALenum error;
				alBufferData(buffers[i],
					channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
					data,
					vorbisLen * sizeof(short) * channels,
					sampleRate
				);

				free(data);

				ALERR;
			}
		}

		return count;
	}

	int xabuffer::XABuffer_close(lua_State *L) {
		// TODO check userdata

		((XALuaHandle*)lua_touserdata(L, 1))->Close(lua_toboolean(L, 2));

		return 0;
	}
};

#endif