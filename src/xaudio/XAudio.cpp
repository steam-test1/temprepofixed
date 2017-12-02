
#include "XAudio.h"

#ifdef ENABLE_XAUDIO

#include "signatures/sigdef.h"
#include "util/util.h"

#include "stb_vorbis.h"

#include <AL/al.h>

#include <string>
#include <map>
#include <vector>

using namespace std;

namespace pd2hook {

	namespace xabuffer {
		static struct XABuffer {
			bool valid;
			ALuint albuffer;
		};

		map<string, XABuffer*> openBuffers;

		static int lX_loadbuffer(lua_State *L) {
			int count = lua_gettop(L);

			ALuint buffers[32];

			if (count > 32) {
				PD2HOOK_LOG_LOG("Attempted to create more than 32 ALbuffers in a single call!");
				return 0;
			}

			vector<string> filenames;
			for (size_t i = 0; i < count; i++) {
				// i+1 because the Lua stack starts at 1, not 0
				filenames.push_back(lua_tostring(L, i + 1));
			}
			lua_settop(L, 0);

			alGenBuffers(count, buffers);
			for (size_t i = 0; i < count; i++) {
				string filename = filenames[i];

				if (openBuffers.count(filename)) {
					XABuffer **ptr = (XABuffer**)lua_newuserdata(L, sizeof(XABuffer*));
					*ptr = openBuffers[filename];
					// TODO don't create buffers for cached stuff
				}
				else {
					XABuffer **ptr = (XABuffer**)lua_newuserdata(L, sizeof(XABuffer*));
					XABuffer *buff = new XABuffer();
					*ptr = buff;
					buff->valid = true;
					buff->albuffer = buffers[i];

					openBuffers[filename] = buff;

					// Set the metatable
					luaL_getmetatable(L, "XAudio.buffer");
					lua_setmetatable(L, -2);

					// Load the contents of the buffer

					ALenum format;
					ALvoid *data;
					ALsizei size;
					ALsizei freq;

					int vorbisLen, channels, sampleRate;
					short *vorbis;

					vorbisLen = stb_vorbis_decode_filename(filename.c_str(),
						&channels,
						&sampleRate,
						&vorbis);

					// Copy the file into our buffer
					// TODO do this in the background
					ALenum error;
					alBufferData(buffers[i],
						channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
						vorbis,
						vorbisLen * sizeof(short),
						sampleRate
					);
					if ((error = alGetError()) != AL_NO_ERROR) {
						throw "alBufferData buffer 0 : " + error;
					}
				}
			}

			return count;
		}

		static int XABuffer_close(lua_State *L) {
			// TODO check userdata

			XABuffer *buff = *(XABuffer**)lua_touserdata(L, 1);
			buff->valid = false;
			alDeleteBuffers(1, &buff->albuffer);

			return 0;
		}
	};

	namespace xasource {
		static struct XASource {
			bool valid;
			ALuint alsource;
		};

		vector<XASource*> openSources;

		static int lX_new_source(lua_State *L) {
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
				XASource **ptr = (XASource**)lua_newuserdata(L, sizeof(XASource*));
				XASource *buff = new XASource();
				*ptr = buff;

				buff->valid = true;
				buff->alsource = sources[i];

				// Set the metatable
				luaL_getmetatable(L, "XAudio.source");
				lua_setmetatable(L, -2);
			}

			return count;
		}

		static int XASource_close(lua_State *L) {
			// TODO check userdata

			XASource *buff = *(XASource**)lua_touserdata(L, 1);
			buff->valid = false;
			alDeleteSources(1, &buff->alsource);
			buff->alsource = -1; // Hopefully crash if it gets used

			// TODO remove from openSources

			// TODO how do we go about deleting buff, if it's in use as another userdata object?

			return 0;
		}

		static int XASource_set_buffer(lua_State *L) {
			XASource *xthis = *(XASource**)lua_touserdata(L, 1);
			// TODO validate 'valid' flag

			bool nil = lua_isnil(L, 2);
			ALuint buffid;
			if (nil) {
				buffid = 0; // NULL Buffer - basically, no audio
			}
			else {
				xabuffer::XABuffer *buff = *(xabuffer::XABuffer**)lua_touserdata(L, 2);
				// TODO validate 'valid' flag
				buffid = buff->albuffer;
			}

			// Attach buffer 0 to source 
			alSourcei(xthis->alsource, AL_BUFFER, buffid);
			ALenum error;
			if ((error = alGetError()) != AL_NO_ERROR) {
				throw string("alSourcei AL_BUFFER 0 : " + error);
			}

			return 0;
		}

		static int XASource_play(lua_State *L) {
			XASource *xthis = *(XASource**)lua_touserdata(L, 1);
			// TODO validate 'valid' flag

			alSourcePlay(xthis->alsource);
			// TODO error checking

			return 0;
		}

		static void set_vector_property(lua_State *L, ALenum type) {
			XASource *xthis = *(XASource**)lua_touserdata(L, 1);
			// TODO validate 'valid' flag

			ALfloat x = lua_tonumber(L, 2);
			ALfloat y = lua_tonumber(L, 3);
			ALfloat z = lua_tonumber(L, 4);

			alSource3f(
				xthis->alsource,
				type,
				x,
				y,
				z
			);
		}

		static int XASource_set_position(lua_State *L) {
			set_vector_property(L, AL_POSITION);
			return 0;
		}

		static int XASource_set_velocity(lua_State *L) {
			set_vector_property(L, AL_VELOCITY);
			return 0;
		}

		static int XASource_set_direction(lua_State *L) {
			// FIXME doesn't *seem* to work?
			set_vector_property(L, AL_DIRECTION);
			return 0;
		}
	};

	namespace xalistener {
		static void set_vector_property(lua_State *L, ALenum type) {
			// TODO we can use the listener property in L.1

			ALfloat x = lua_tonumber(L, 2);
			ALfloat y = lua_tonumber(L, 3);
			ALfloat z = lua_tonumber(L, 4);

			alListener3f(
				type,
				x,
				y,
				z
			);
		}

		static int XAListener_set_position(lua_State *L) {
			set_vector_property(L, AL_POSITION);
			return 0;
		}

		static int XAListener_set_velocity(lua_State *L) {
			set_vector_property(L, AL_VELOCITY);
			return 0;
		}

		static int XAListener_set_orientation(lua_State *L) {
			// TODO we can use the listener property in L.1

			// Forward vector
			ALfloat xfwd = lua_tonumber(L, 2);
			ALfloat yfwd = lua_tonumber(L, 3);
			ALfloat zfwd = lua_tonumber(L, 4);

			// Up vector
			ALfloat xup = lua_tonumber(L, 5);
			ALfloat yup = lua_tonumber(L, 6);
			ALfloat zup = lua_tonumber(L, 7);

			ALfloat orientation[6] = {
				xfwd, yfwd, zfwd,
				xup, yup, zup
			};

			alListenerfv(AL_ORIENTATION, orientation);

			return 0;
		}

		static void lua_register(lua_State *L) {
			// blt.xaudio.listener table
			luaL_Reg lib[] = {
				{ "setposition", xalistener::XAListener_set_position },
				{ "setvelocity", xalistener::XAListener_set_velocity },
				{ "setorientation", xalistener::XAListener_set_orientation },
				{ NULL, NULL }
			};

			// Make a new table and populate it with XAudio stuff
			lua_newtable(L);
			luaI_openlib(L, NULL, lib, 0);

			// Put that table into the BLT table, calling it XAudio. This removes said table from the stack.
			lua_setfield(L, -2, "listener");
		}
	};

	static int lX_setup(lua_State *L) {
		try {
			XAudio::GetXAudioInstance();
		}
		catch (string msg) {
			PD2HOOK_LOG_ERROR("Exception while loading XAudio API: " + msg);
			lua_pushboolean(L, false);
			return 1;
		}
		lua_pushboolean(L, true);
		return 1;
	}

	XAudio::XAudio() {
		struct stat statbuf;

		dev = alcOpenDevice(NULL);
		if (!dev) {
			throw string("Cannot open OpenAL Device");
		}
		ctx = alcCreateContext(dev, NULL);
		alcMakeContextCurrent(ctx);
		if (!ctx) {
			throw string("Could not create OpenAL Context");
		}

		PD2HOOK_LOG_LOG("Loaded OpenAL XAudio API");
	}

	XAudio::~XAudio() {
		PD2HOOK_LOG_LOG("Closing OpenAL XAudio API");

		// Delete all sources
		// Do this first, otherwise buffers might not be deleted properly
		for (auto source : xasource::openSources) {
			if (source->valid) {
				alDeleteSources(1, &source->alsource);
			}

			delete source;
		}

		// To remove dangling pointers
		// Should not be used again, but just to be safe
		xasource::openSources.clear();

		// Delete all buffers
		for (auto const& pair : xabuffer::openBuffers) {
			xabuffer::XABuffer *buff = pair.second;

			if (buff->valid) {
				alDeleteBuffers(1, &buff->albuffer);
			}

			delete buff;
		}

		// Same as above
		xabuffer::openBuffers.clear();

		// TODO: Make sure above works properly.

		// Close the OpenAL context/device
		alcMakeContextCurrent(NULL);
		alcDestroyContext(ctx);
		alcCloseDevice(dev);
	}

	XAudio* XAudio::GetXAudioInstance() {
		static XAudio audio;
		return &audio;
	}

	void XAudio::Register(void * state) {
		lua_State *L = (lua_State*)state;

		// Buffer metatable
		luaL_Reg XABufferLib[] = {
			{ "close", xabuffer::XABuffer_close },
			{ NULL, NULL }
		};

		luaL_newmetatable(L, "XAudio.buffer");

		lua_pushstring(L, "__index");
		lua_pushvalue(L, -2);  /* pushes the metatable */
		lua_settable(L, -3);  /* metatable.__index = metatable */

		luaI_openlib(L, NULL, XABufferLib, 0);
		lua_pop(L, 1);

		// Source metatable
		luaL_Reg XASourceLib[] = {
			{ "close", xasource::XASource_close },
			{ "setbuffer", xasource::XASource_set_buffer },
			{ "play", xasource::XASource_play },
			{ "setposition", xasource::XASource_set_position },
			{ "setvelocity", xasource::XASource_set_velocity },
			{ "setdirection", xasource::XASource_set_direction },
			{ NULL, NULL }
		};

		luaL_newmetatable(L, "XAudio.source");

		lua_pushstring(L, "__index");
		lua_pushvalue(L, -2);  /* pushes the metatable */
		lua_settable(L, -3);  /* metatable.__index = metatable */

		luaI_openlib(L, NULL, XASourceLib, 0);
		lua_pop(L, 1);

		// blt.xaudio table
		luaL_Reg lib[] = {
			{ "setup", lX_setup },
			{ "loadbuffer", xabuffer::lX_loadbuffer },
			{ "newsource", xasource::lX_new_source },
			{ NULL, NULL }
		};

		// Grab the BLT table
		lua_getglobal(L, "blt");

		// Make a new table and populate it with XAudio stuff
		lua_newtable(L);
		luaI_openlib(L, NULL, lib, 0);

		// Add the blt.xaudio.listener table
		xalistener::lua_register(L);

		// Put that table into the BLT table, calling it XAudio. This removes said table from the stack.
		lua_setfield(L, -2, "xaudio");

		// Remove the BLT table from the stack.
		lua_pop(L, 1);
	}

}

#endif // ENABLE_XAUDIO
