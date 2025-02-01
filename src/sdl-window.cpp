#include "sdl-window.h"
#include "android-application.hpp"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

bool SdlAppWindow::init() {
	SDL_SetAppMetadata("Silene", "0.0.1", "dev.xyze.silene");
	
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		spdlog::info("failed to initialize SDL: {}", SDL_GetError());
		return false;
	}

	// SDL_GL_LoadLibrary(nullptr);

#ifdef SILENE_USE_EGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	auto window_title = fmt::format("{} (Silene)", _config.title_name);	
	auto window = SDL_CreateWindow(window_title.c_str(), 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (window == nullptr) {
		spdlog::info("failed to create SDL window: {}", SDL_GetError());
		return false;
	}
	
	_window = window;

	auto glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr) {
		spdlog::info("failed to create GL context: {}", SDL_GetError());
		return false;
	}

	if (!SDL_GL_SetSwapInterval(-1)) {
		if (!SDL_GL_SetSwapInterval(1)) {
			spdlog::warn("Failed to enable vertical sync: {}", SDL_GetError());
		}
	}

#ifndef SILENE_USE_EGL
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
		spdlog::error("Failed to initialize GLAD: {}", SDL_GetError());
		return false;
	}
#endif

	auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

	spdlog::info("gl information: {} ({}) - {}", renderer, vendor, version);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsLight();

	ImGui_ImplSDL3_InitForOpenGL(window, glContext);

	#ifdef SILENE_USE_EGL
	ImGui_ImplOpenGL3_Init("#version 300 es");
	#else
	ImGui_ImplOpenGL3_Init("#version 120");
	#endif

	if (!_config.keybind_file.empty()) {
		_keybind_manager = std::make_unique<KeybindManager>(_config.keybind_file);
	}
	
	_is_first_frame = true;

	return true;
}

void SdlAppWindow::handle_key(SDL_KeyboardEvent& event) {
	if (_keybind_manager != nullptr) {
		auto key_name = SDL_GetKeyName(event.key);
		if (auto touch = _keybind_manager->handle(key_name)) {
			auto [x, y] = touch.value();

			application().send_touch(event.down, {
				1,
				static_cast<float>(x),
				static_cast<float>(y)
			});

			return;
		}
	}

	if (!event.down) {
		return;
	}

	switch (event.key) {
		case SDLK_BACKSPACE:
			application().send_ime_delete();
			break;
		case SDLK_ESCAPE:
			application().send_keydown(4 /* AKEYCODE_BACK */);
			break;
		case SDLK_SPACE:
			application().send_ime_insert(" ");
			break;
		default: {
			auto key_name = SDL_GetKeyName(event.key);

			application().send_ime_insert(key_name);
			break;
		}
	}
}

void SdlAppWindow::handle_event(SDL_Event* event) {
	ImGui_ImplSDL3_ProcessEvent(event);

	switch (event->type) {
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			auto& button = event->button;
			if (button.button != SDL_BUTTON_LEFT) {
				return;
			}

			application().send_touch(button.down, {
				1,
				static_cast<float>(button.x),
				static_cast<float>(button.y)
			});

			break;
		}
		case SDL_EVENT_MOUSE_MOTION: {
			auto& motion = event->motion;
			application().move_touches({{
				1,
				static_cast<float>(motion.x),
				static_cast<float>(motion.y)
			}});
			break;
		}
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP: {
			this->handle_key(event->key);
			break;
		}
	}
}

void SdlAppWindow::on_quit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void SdlAppWindow::main_loop() {
	auto current_tick = SDL_GetTicks();
	auto tick_difference = current_tick - _last_tick;
	if (tick_difference >= 1000) {
		_last_tick = SDL_GetTicks();	
		_fps = static_cast<float>(_frame_counter * 1000) / tick_difference;
		_frame_counter = 0;
	} else {
		_frame_counter++;
	}

	if (_is_first_frame) {
		int width, height;
		SDL_GetWindowSize(_window, &width, &height);

		application().init_game(width, height);
		_is_first_frame = false;
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin("Info Dialog", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("FPS: %.0f", _fps);

		if (_config.show_cursor_pos) {
			float xpos, ypos;
			SDL_GetMouseState(&xpos, &ypos);

			ImGui::Text("X: %.0f | Y: %.0f", xpos, ypos);
		}
	}

	ImGui::End();

	ImGui::Render();

	application().draw_frame();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	SDL_GL_SwapWindow(_window);
}

SdlAppWindow::SdlAppWindow(std::unique_ptr<AndroidApplication>&& app, WindowConfig config)
	: BaseWindow(*app), _config{config}, _application{std::move(app)} {}
