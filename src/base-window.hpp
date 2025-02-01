#pragma once

#ifndef _BASE_WINDOW_H
#define _BASE_WINDOW_H

#include <utility>

class AndroidApplication;

class BaseWindow {
private:
	AndroidApplication& _application;

protected:
	AndroidApplication& application() const {
		return this->_application;
	}

public:
	virtual ~BaseWindow() = default;

	virtual bool init() = 0;
	virtual void main_loop() = 0;

	BaseWindow(AndroidApplication& application) : _application{application} {}
};

#endif
