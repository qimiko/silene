#pragma once

#ifndef _AUDIO_ENGINE_H

#include <soloud.h>

#include "environment.h"

class AudioEngine {
	SoLoud::Soloud _engine{};

public:
	void init_audio();
	void init_symbols(Environment& env);

	~AudioEngine();
};

#endif
