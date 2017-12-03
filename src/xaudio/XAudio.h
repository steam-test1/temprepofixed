#pragma once

#define ENABLE_XAUDIO

#ifdef ENABLE_XAUDIO

// So we don't have to include the OpenAL headers now
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

namespace pd2hook {

	class XAudio {
	public:
		static void Register(void *state);

		// Please don't use, for internal use only
		static XAudio* GetXAudioInstance();
	private:
		XAudio();
		~XAudio();

		ALCdevice *dev;
		ALCcontext *ctx;
	};

};

#endif
