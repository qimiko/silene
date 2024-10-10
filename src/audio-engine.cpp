#include "audio-engine.h"

#include "syscall-handler.hpp"
#include "syscall-translator.hpp"

namespace {
	void emu_fmod_system_update(Environment& env) {
		// we don't need to update anything for soloud
		return;
	}
}

void AudioEngine::init_audio() {
	_engine.init(SoLoud::Soloud::CLIP_ROUNDOFF | SoLoud::Soloud::ENABLE_VISUALIZATION);
}

void AudioEngine::init_symbols(Environment& env) {
	REGISTER_FN_RN(env, emu_fmod_system_update, "_ZN4FMOD6System6updateEv");
}


AudioEngine::~AudioEngine() {
	_engine.deinit();
}

