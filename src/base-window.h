#pragma once

#ifndef _BASE_WINDOW_H
#define _BASE_WINDOW_H

#include <utility>

class AndroidApplication;

class BaseWindow {
public:
	virtual std::pair<float, float> framebuffer_size() = 0;
	virtual bool init(AndroidApplication& application) = 0;
	virtual void main_loop() = 0;
};

#endif
