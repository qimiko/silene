#include "android-window.h"

#include <vector>
#include <spdlog/spdlog.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <swappy/swappyGL.h>

#include <imgui.h>
#include <backends/imgui_impl_android.h>
#include <backends/imgui_impl_opengl3.h>

#include "android-application.hpp"

// i think a lot of the code here comes from cocosv3's native backend
// but it's been like two years since i wrote it, sorry...

bool AndroidWindow::init_display() {
	if (_egl_display != EGL_NO_DISPLAY) {
		return true;
	}

	spdlog::info("running display init");

	_egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (EGL_FALSE == eglInitialize(_egl_display, nullptr, nullptr)) {
		spdlog::error("failed to init display, error {}", eglGetError());
		return false;
	}

	return true;
}

bool AndroidWindow::init_surface() {
	if (_egl_surface != EGL_NO_SURFACE) {
		return true;
	}

	spdlog::info("running surface init");

	EGLint num_configs = -1;
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_BLUE_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_RED_SIZE, 5,
		EGL_DEPTH_SIZE, 16,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	eglChooseConfig(_egl_display, attribs, &_egl_config, 1, &num_configs);
	_egl_surface = eglCreateWindowSurface(_egl_display, _egl_config, _window->window, nullptr);

	auto width = 0, height = 0;
	eglQuerySurface(_egl_display, _egl_surface, EGL_WIDTH, &width);
	eglQuerySurface(_egl_display, _egl_surface, EGL_HEIGHT, &height);

	if (_egl_surface == EGL_NO_SURFACE) {
			spdlog::error("Failed to create EGL surface, EGL error {}", eglGetError());
			return false;
	}

	return true;
}

bool AndroidWindow::init_context() {
	if (_egl_context != EGL_NO_CONTEXT) {
		return true;
	}

	spdlog::info("running context init");

	EGLint attrib_list[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	_egl_context = eglCreateContext(_egl_display, _egl_config, nullptr, attrib_list);
	if (_egl_context == EGL_NO_CONTEXT) {
		spdlog::error("Failed to create EGL context, EGL error {}", eglGetError());
		return false;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	return true;
}

void AndroidWindow::init_render_full() {
	if (_egl_display == EGL_NO_DISPLAY || _egl_surface == EGL_NO_SURFACE || _egl_context == EGL_NO_CONTEXT) {
		spdlog::info("running render init");

		init_display();
		init_surface();
		init_context();

		if (eglMakeCurrent(_egl_display, _egl_surface, _egl_surface, _egl_context) == EGL_FALSE) {
			spdlog::error("eglMakeCurrent failed, EGL error {}", eglGetError());
		}

		eglQuerySurface(_egl_display, _egl_surface, EGL_WIDTH, &_surface_width);
		eglQuerySurface(_egl_display, _egl_surface, EGL_HEIGHT, &_surface_height);

		auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

		spdlog::info("gl information: {} ({}) - {}", renderer, vendor, version);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsLight();

		ImGui_ImplAndroid_Init(_window->window);

		#ifdef SILENE_USE_EGL
		ImGui_ImplOpenGL3_Init("#version 300 es");
		#else
		ImGui_ImplOpenGL3_Init("#version 120");
		#endif
	}
}

void AndroidWindow::kill_surface() {
	if (_egl_surface != EGL_NO_SURFACE) {
		eglDestroySurface(_egl_display, _egl_surface);
		_egl_surface = EGL_NO_SURFACE;
	}
}

void AndroidWindow::kill_context() {
	if (_egl_context != EGL_NO_CONTEXT) {
		eglDestroyContext(_egl_display, _egl_context);
		_egl_context = EGL_NO_CONTEXT;
	}
}

void AndroidWindow::kill_display() {
	// causes context and surface to go away too, if they are there
	eglMakeCurrent(_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	kill_context();
	kill_surface();

	if (_egl_display != EGL_NO_DISPLAY) {
		eglTerminate(_egl_display);
		_egl_display = EGL_NO_DISPLAY;
	}
}

void AndroidWindow::on_motion_event(GameActivityMotionEvent* motion_event) {
	int action = motion_event->action;
	int action_masked = action & AMOTION_EVENT_ACTION_MASK;
	int ptr_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

	switch (action_masked) {
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
		case AMOTION_EVENT_ACTION_DOWN: {
			auto pointer = &motion_event->pointers[ptr_index];
			auto pointer_id = pointer->id;
			auto x =  GameActivityPointerAxes_getX(pointer);
			auto y = GameActivityPointerAxes_getY(pointer);

			application().send_touch(true, {
				static_cast<std::uint32_t>(pointer_id), x, y
			});

			break;
		}
		case AMOTION_EVENT_ACTION_POINTER_UP:
		case AMOTION_EVENT_ACTION_UP: {
			auto pointer = &motion_event->pointers[ptr_index];
			auto pointer_id = pointer->id;
			auto x =  GameActivityPointerAxes_getX(pointer);
			auto y = GameActivityPointerAxes_getY(pointer);

			application().send_touch(false, {
				static_cast<std::uint32_t>(pointer_id), x, y
			});
			break;
		}
		case AMOTION_EVENT_ACTION_MOVE: {
			auto count = motion_event->pointerCount;

			std::vector<AndroidApplication::TouchData> touches{};
			touches.reserve(count);

			for (uint32_t p = 0; p < count; ++p) {
				auto pointer = &motion_event->pointers[p];
				touches.push_back({
					static_cast<std::uint32_t>(pointer->id),
					GameActivityPointerAxes_getX(pointer),
					GameActivityPointerAxes_getY(pointer)
				});
			}

			application().move_touches(touches);
			break;
		}
		case AMOTION_EVENT_ACTION_CANCEL:
			spdlog::info("TODO: touches cancelled");
			break;
	}
}

void AndroidWindow::on_key_event(GameActivityKeyEvent* key_event) {
	int action = key_event->action;
	int action_masked = action & AMOTION_EVENT_ACTION_MASK;

	switch (action_masked) {
		case AMOTION_EVENT_ACTION_DOWN:
			application().send_keydown(key_event->keyCode);
			break;
		default:
			break;
	}
}

void AndroidWindow::loop_check_input() {
	auto input_buffer = android_app_swap_input_buffers(_window);
	if (input_buffer == nullptr) return;

	if (input_buffer->motionEventsCount != 0) {
		for (uint64_t i = 0; i < input_buffer->motionEventsCount; ++i) {
			auto motion_event = &input_buffer->motionEvents[i];
			on_motion_event(motion_event);
		}
		android_app_clear_motion_events(input_buffer);
	}

	if (input_buffer->keyEventsCount != 0) {
		for (uint64_t i = 0; i < input_buffer->keyEventsCount; ++i) {
			auto key_event = &input_buffer->keyEvents[i];
			on_key_event(key_event);
		}
		android_app_clear_key_events(input_buffer);
	}
}

void AndroidWindow::handle_command(std::int32_t cmd) {
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			spdlog::info("APP_CMD_INIT_WINDOW");
			if (_window->window != nullptr) {
				_has_window = true;
				SwappyGL_setWindow(_window->window);
				_has_focus = true;
			}
			break;
		case APP_CMD_TERM_WINDOW:
			spdlog::info("APP_CMD_TERM_WINDOW");
			this->kill_display();

			SwappyGL_destroy();
			_window->activity->vm->DetachCurrentThread();
			_has_window = false;

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplAndroid_Shutdown();
			ImGui::DestroyContext();

			break;
		case APP_CMD_GAINED_FOCUS:
			spdlog::info("APP_CMD_GAINED_FOCUS");
			_has_focus = true;
			break;
		case APP_CMD_LOST_FOCUS:
			spdlog::info("APP_CMD_LOST_FOCUS");
			_has_focus = false;
			break;
		case APP_CMD_STOP:
			spdlog::info("NativeEngine: APP_CMD_STOP");
			_is_visible = false;
			break;
		case APP_CMD_START:
			spdlog::info("APP_CMD_START");
			_is_visible = true;
			break;
		default:
			spdlog::info("unhandled app cmd {}", cmd);
			break;
	}
}

void AndroidWindow::handle_cmd_proxy(struct android_app* window, std::int32_t cmd) {
	auto android_window = reinterpret_cast<AndroidWindow*>(window->userData);
	android_window->handle_command(cmd);
}

bool AndroidWindow::is_animating() const {
	return _has_focus && _is_visible && _has_window;
}

bool AndroidWindow::init() {
	// annoyingly, we don't actually know if we have a window by this point, so no opengl is allowed
	// but we can setup our inputs

  _window->userData = this;
  _window->onAppCmd = handle_cmd_proxy;

	if (_jni != nullptr) {
		return true;
	}

	auto vm = _window->activity->vm;
	vm->AttachCurrentThread(&_jni, nullptr);
	if (_jni == nullptr) {
		spdlog::error("JNI AttachCurrentThread failed");
		return false;
	}

	SwappyGL_init(_jni, _window->activity->javaGameActivity);

	return true;
}

void AndroidWindow::run_frame() {
	init_render_full();

	if (_is_first_frame) {
		application().init_game(_surface_width, _surface_height);
		_is_first_frame = false;
	}

	if (_egl_display == nullptr || _egl_surface == nullptr) {
		return;
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplAndroid_NewFrame();
	ImGui::NewFrame();

	// not a lot of imgui code here yet

	ImGui::Render();

	application().draw_frame();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SwappyGL_swap(_egl_display, _egl_surface);
}

void AndroidWindow::main_loop() {
	while (true) {
		int events = 0;
		struct android_poll_source* source = nullptr;

		while ((ALooper_pollOnce(is_animating() ? 0 : -1, nullptr, &events, reinterpret_cast<void **>(&source))) > ALOOPER_POLL_TIMEOUT) {
			if (source != nullptr) {
				source->process(source->app, source);
			}

			if (_window->destroyRequested) {
				return;
			}
		}

		loop_check_input();

		if (is_animating()) {
			run_frame();
		}
	}
}

void AndroidWindow::show_dialog(std::string message) {
	auto clazz = _jni->GetObjectClass(_window->activity->javaGameActivity);
	auto method_id = _jni->GetMethodID(clazz, "showDialog", "(Ljava/lang/String;)V");

	auto message_ref = _jni->NewStringUTF(message.c_str());
	_jni->CallVoidMethod(_window->activity->javaGameActivity, method_id, message_ref);
	_jni->DeleteLocalRef(message_ref);
}
