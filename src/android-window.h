#pragma once

#ifndef _ANDROID_WINDOW_H
#define _ANDROID_WINDOW_H

#include <EGL/egl.h>

#include "base-window.hpp"

#include <game-activity/native_app_glue/android_native_app_glue.h>

class AndroidWindow : public BaseWindow {
private:
	struct android_app* _window;

	EGLDisplay _egl_display{EGL_NO_DISPLAY};
	EGLSurface _egl_surface{EGL_NO_SURFACE};
	EGLContext _egl_context{EGL_NO_CONTEXT};
	EGLConfig _egl_config{nullptr};

	JNIEnv* _jni = nullptr;

	bool _has_focus{false};
	bool _is_visible{false};
	bool _has_window{false};
	bool _is_first_frame{true};

	int _surface_width{0};
	int _surface_height{0};

	bool init_display();
	bool init_surface();
	bool init_context();
	void init_render_full();

	void kill_surface();
	void kill_context();
	void kill_display();

	bool is_animating() const;

	void on_motion_event(GameActivityMotionEvent* motion_event);
	void on_key_event(GameActivityKeyEvent* key_event);
	void loop_check_input();
	void run_frame();

	void handle_command(std::int32_t cmd);
	static void handle_cmd_proxy(struct android_app* window, std::int32_t cmd);

public:
	virtual bool init() override;
	virtual void main_loop() override;

	void show_dialog(std::string message);

	AndroidWindow(struct android_app* window, AndroidApplication& app)
		: BaseWindow(app), _window{window} {}
};

#endif
