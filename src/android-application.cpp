#include "android-application.hpp"

#include <dynarmic/interface/A32/a32.h>
#include <dynarmic/interface/A32/config.h>

#include "android-coprocessor.hpp"
#include "zip-file.h"

void AndroidApplication::init_jni() {
	if (this->program_loader().has_symbol("JNI_OnLoad")) {
		auto jvm_ptr = this->jni().get_vm_ptr();
		_env.call_symbol<void>("JNI_OnLoad", jvm_ptr);
	}
}

void AndroidApplication::load_library(Elf::File& lib) {
	auto header = lib.header();
	if (header->type != Elf::Type::Shared || header->machine != Elf::Machine::Armv7) {
		throw std::runtime_error("library cannot be loaded: not shared or not armv7");
	}

	this->_state.program_loader.map_elf(lib);
}

void AndroidApplication::finalize_libraries() {
	auto init_fns = this->program_loader().get_init_functions();

	for (const auto& fn : init_fns) {
		if (fn == 0 || fn == 0xffff'ffff) {
			continue;
		}

		this->_env.run_func(fn);
	}

	this->program_loader().clear_init_functions();
}

void AndroidApplication::init() {
	this->init_memory();

	if (_config.debug) {
		_env.begin_debugging();
	}
}

void AndroidApplication::create_processor_with_func(std::uint32_t start_addr, std::uint32_t arg, std::uint32_t id) {
	auto& env = _envs.at(id);
	env.current_cpu()->Regs()[0] = arg;
	env.run_func(start_addr);

	{
		// this processor is unneeded, so remove it ig
		// otherwise, keep it around to read the return value
		if (!_unclaimed_threads.contains(id)) {
			std::scoped_lock lk{_envs_mutex};
			_envs.erase(id);
		}
	}
}

std::uint32_t AndroidApplication::create_thread(std::uint32_t start_addr, std::uint32_t arg) {
	std::scoped_lock lk{_envs_mutex};

	auto id = _last_tid++;
	_envs.try_emplace(id, *this, _state, &_monitor, id);

	// keep the thread alive so join/detach can be called on it
	_unclaimed_threads.try_emplace(id, &AndroidApplication::create_processor_with_func, this, start_addr, arg, id);

	return id;
}

bool AndroidApplication::detach_thread(std::uint32_t thread_id) {
	if (!_unclaimed_threads.contains(thread_id)) {
		return false;
	}

	auto& thread = _unclaimed_threads.at(thread_id);
	thread.detach();

	{
		std::scoped_lock lk{_envs_mutex};
		_unclaimed_threads.erase(thread_id);

		if (!_envs.at(thread_id).is_active()) {
			_envs.erase(thread_id);
		}
	}

	return true;
}

bool AndroidApplication::join_thread(std::uint32_t thread_id, std::uint32_t exit_value) {
	if (!_unclaimed_threads.contains(thread_id)) {
		return false;
	}

	auto& thread = _unclaimed_threads.at(thread_id);
	thread.join();

	auto r_ptr = 0;
	{
		std::scoped_lock lk{_envs_mutex};
		_unclaimed_threads.erase(thread_id);

		r_ptr = _envs.at(thread_id).current_cpu()->Regs()[0];
		_envs.erase(thread_id);
	}

	if (exit_value != 0) {
		memory_manager().write_word(exit_value, r_ptr);
	}

	return true;
}

void AndroidApplication::init_memory() {
	this->memory_manager().allocate_stack();

	// block off the first page for "reasons" and then allocate more space for our returns
	this->memory_manager().allocate(PagedMemory::EMU_PAGE_SIZE + 0xff);

	// currently nullptr should be null
	// maybe later add proper null page handling?
	auto page_offs = PagedMemory::EMU_PAGE_SIZE;

	// it should halt the cpu here
	this->memory_manager().write_halfword(page_offs + 0x10, 0xdf01); // svc #0x1
	this->program_loader().set_return_stub_addr(page_offs + 0x11);

	// fallback symbol handler
	this->memory_manager().write_halfword(page_offs + 0x20, 0xdf02); // svc #0x2
	this->memory_manager().write_halfword(page_offs + 0x22, 0x4770); // bx lr

	this->program_loader().set_symbol_fallback_addr(page_offs + 0x21);

	this->libc().pre_init(*this);
	this->jni().pre_init(*this);
}

void AndroidApplication::init_game(int width, int height) {
	this->libc().expose_file("/application_resources.apk", _config.resources);

	// first arg should be a jstring to the path
	auto jni_env_ptr = this->jni().get_env_ptr();
	auto path_string = this->jni().create_string_ref("/application_resources.apk");

	if (this->program_loader().has_symbol("Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetPaths")) {
		// this symbol was used on older versions of cocos (but are otherwise identical)
		_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetPaths", jni_env_ptr, 0, path_string);
	} else {
		_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetApkPath", jni_env_ptr, 0, path_string);
	}

	this->jni().remove_ref(path_string);

	_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit", jni_env_ptr, 0, width, height);
	spdlog::info("init finished");
}

AndroidApplication::AndroidApplication(ApplicationConfig config)
	: StateHolder{_state}, _config{config}, _env{*this, _state, &_monitor, 0} {}

void AndroidApplication::draw_frame() {
	auto jni_env_ptr = this->jni().get_env_ptr();
	_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender", jni_env_ptr, 0);
}

void AndroidApplication::send_touch(bool is_push, TouchData data) {
	auto jni_env_ptr = this->jni().get_env_ptr();

	if (is_push) {
		_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin",
			jni_env_ptr, 0, data.id, data.x, data.y);
	} else {
		_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd",
			jni_env_ptr, 0, data.id, data.x, data.y);
	}
}

void AndroidApplication::move_touches(const std::vector<TouchData>& touches) {
	auto jni_env_ptr = this->jni().get_env_ptr();

	std::vector<int> ids{};
	std::vector<float> xs{};
	std::vector<float> ys{};

	for (const auto& touch : touches) {
		ids.push_back(touch.id);
		xs.push_back(touch.x);
		ys.push_back(touch.y);
	}

	auto ids_ref = this->jni().create_ref(ids);
	auto xs_ref = this->jni().create_ref(xs);
	auto ys_ref = this->jni().create_ref(ys);

	_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove", jni_env_ptr, 0, ids_ref, xs_ref, ys_ref);
}

void AndroidApplication::send_ime_insert(std::string data) {
	if (!this->program_loader().has_symbol("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInsertText")) {
		return;
	}

	auto jni_env_ptr = this->jni().get_env_ptr();
	auto data_ref = this->jni().create_string_ref(data);
	_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInsertText", jni_env_ptr, 0, data_ref);
}

void AndroidApplication::send_ime_delete() {
	if (!this->program_loader().has_symbol("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeDeleteBackward")) {
		return;
	}

	auto jni_env_ptr = this->jni().get_env_ptr();
	_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeDeleteBackward", jni_env_ptr, 0);
}

void AndroidApplication::send_keydown(int android_keycode) {
	auto jni_env_ptr = this->jni().get_env_ptr();
	_env.call_symbol<void>("Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyDown", jni_env_ptr, 0, android_keycode);
}
