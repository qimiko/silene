#pragma once

#ifndef _GL_WRAP_H
#define _GL_WRAP_H

#include "../environment.h"

#ifdef SILENE_USE_EGL
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#else
#include <glad/glad.h>
#endif

std::uint32_t emu_glGetString(Environment& env, std::uint32_t name) {
	auto r = reinterpret_cast<const char*>(glGetString(name));
	if (r == 0) {
		return 0;
	}

	// TODO: this is a memory leak.
	auto r_len = strlen(r);
	auto r_ptr = env.libc().allocate_memory(r_len + 1);

	auto str = env.memory_manager().read_bytes<char>(r_ptr);
	std::memcpy(str, r, r_len + 1);

	spdlog::trace("glGetString(name: {}) -> {}", name, glGetError());

	return r_ptr;
}

void emu_glGetIntegerv(Environment& env, std::uint32_t name, std::uint32_t data_ptr) {
	int* data = nullptr;
	if (data_ptr != 0) {
		data = env.memory_manager().read_bytes<int>(data_ptr);
	}

	glGetIntegerv(name, data);
	spdlog::trace("glGetIntegerv(name: {}, data: {:#x}) -> {}", name, data_ptr, glGetError());
}

void emu_glGetFloatv(Environment& env, std::uint32_t name, std::uint32_t data_ptr) {
	float* data = nullptr;
	if (data_ptr != 0) {
		data = env.memory_manager().read_bytes<float>(data_ptr);
	}

	glGetFloatv(name, data);
	spdlog::trace("glGetFloatv(name: {}, data: {:#x}) -> {}", name, data_ptr, glGetError());
}

void emu_glPixelStorei(Environment& env, std::uint32_t name, std::int32_t param) {
	glPixelStorei(name, param);

	spdlog::trace("glPixelStorei(name: {}, param: {}) -> {}", name, param, glGetError());
}

std::uint32_t emu_glCreateShader(Environment& env, std::uint32_t type) {
	auto r = glCreateShader(type);

	spdlog::trace("glCreateShader(type: {}) => {} -> {}", type, r, glGetError());

	return r;
}

void emu_glShaderSource(Environment& env, std::uint32_t shader, std::uint32_t count, std::uint32_t str_ptrs, std::uint32_t len_ptr) {
	// this one's good, actually
	// we have to translate the 2d array that is str_ptrs

	auto str_ptr = env.memory_manager().read_bytes<int>(str_ptrs);
	int* len = nullptr;

	if (len_ptr != 0) {
		len = env.memory_manager().read_bytes<int>(len_ptr);
	}

	std::vector<char*> str_ptr_tr{};
	str_ptr_tr.reserve(count);

	for (auto i = 0u; i < count; i++) {
		auto ptr = env.memory_manager().read_bytes<char>(str_ptr[i]);

#ifndef SILENE_USE_EGL
		// desktop opengl doesn't support precision
		if (std::strcmp("precision highp float;\n", ptr) == 0 || std::strcmp("precision mediump float;\n", ptr) == 0) {
			continue;
		}
#endif

		str_ptr_tr.push_back(ptr);
	}

	glShaderSource(shader, str_ptr_tr.size(), str_ptr_tr.data(), len);

	spdlog::trace("glShaderSource(shader: {}, count: {}, strs: {:#x}, len: {:#x}) -> {}", shader, count, str_ptrs, len_ptr, glGetError());
}

void emu_glGetShaderSource(Environment& env, std::uint32_t shader, std::uint32_t buf_size, std::uint32_t length_ptr, std::uint32_t source_ptr) {
	std::int32_t* length = nullptr;
	auto source = env.memory_manager().read_bytes<char>(source_ptr);

	if (length_ptr != 0) {
		length = env.memory_manager().read_bytes<std::int32_t>(length_ptr);
	}

	glGetShaderSource(shader, buf_size, length, source);

	spdlog::trace("glGetShaderSource(shader: {}, buf_size: {}, length: {:#x}, source: {:#x}) -> {}", shader, buf_size, length_ptr, source_ptr, glGetError());
}

void emu_glCompileShader(Environment& env, std::uint32_t shader) {
	glCompileShader(shader);

	spdlog::trace("glCompileShader(shader: {}) -> {}", shader, glGetError());
}

void emu_glGetShaderiv(Environment& env, std::uint32_t shader, std::uint32_t name, std::uint32_t data_ptr) {
	int* data = nullptr;
	if (data_ptr != 0) {
		data = env.memory_manager().read_bytes<int>(data_ptr);
	}

	glGetShaderiv(shader, name, data);

	spdlog::trace("glGetShaderiv(shader: {}, name: {}, data: {:#x} => {}) -> {}", shader, name, data_ptr, *data, glGetError());
}

void emu_glGetShaderInfoLog(Environment& env, std::uint32_t shader, std::uint32_t max_length, std::uint32_t length_ptr, std::uint32_t info_log_ptr) {
	int* length = nullptr;
	if (length_ptr != 0) {
		length = env.memory_manager().read_bytes<int>(length_ptr);
	}

	auto info_log = env.memory_manager().read_bytes<char>(info_log_ptr);

	glGetShaderInfoLog(shader, max_length, length, info_log);

	spdlog::trace("glGetShaderInfoLog(shader: {}, max_length: {}, length_ptr: {:#x}, info_log_ptr: {:#x} => {}) -> {}", shader, max_length, length_ptr, info_log_ptr, *info_log, glGetError());
}

std::uint32_t emu_glCreateProgram(Environment& env) {
	return glCreateProgram();

	spdlog::trace("glCreateProgram() -> {}", glGetError());
}

void emu_glAttachShader(Environment& env, std::uint32_t program, std::uint32_t shader) {
	glAttachShader(program, shader);

	spdlog::trace("glAttachShader(program: {}, shader: {}) -> {}", program, shader, glGetError());
}

void emu_glBindAttribLocation(Environment& env, std::uint32_t program, std::uint32_t index, std::uint32_t name_ptr) {
	char* name = nullptr;
	if (name_ptr != 0) {
		name = env.memory_manager().read_bytes<char>(name_ptr);
	}

	glBindAttribLocation(program, index, name);

	spdlog::trace("glBindAttribLocation(program: {}, index: {}, name: {}) -> {}", program, index, name, glGetError());
}

void emu_glLinkProgram(Environment& env, std::uint32_t program) {
	glLinkProgram(program);

	spdlog::trace("glLinkProgram(program: {}) -> {}", program, glGetError());
}

void emu_glDeleteShader(Environment& env, std::uint32_t shader) {
	glDeleteShader(shader);

	spdlog::trace("glDeleteShader(shader: {}) -> {}", shader, glGetError());
}

void emu_glDeleteBuffers(Environment& env, std::uint32_t n, std::uint32_t buffers_ptr) {
	auto buffers = env.memory_manager().read_bytes<std::uint32_t>(buffers_ptr);
	glDeleteBuffers(n, buffers);

	spdlog::trace("glDeleteBuffers(n: {}, buffers: {:#x}) -> {}", n, buffers_ptr, glGetError());
}

std::int32_t emu_glGetUniformLocation(Environment& env, std::uint32_t program, std::uint32_t name_ptr) {
	auto name = env.memory_manager().read_bytes<char>(name_ptr);
	return glGetUniformLocation(program, name);

	spdlog::trace("glGetUniformLocation(program: {}, name: {}) -> {}", program, name, glGetError());
}

void emu_glEnable(Environment& env, std::uint32_t cap) {
	glEnable(cap);
}

void emu_glDisable(Environment& env, std::uint32_t cap) {
	glDisable(cap);
}

void emu_glScissor(Environment& env, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height) {
	glScissor(x, y, width, height);
}

void emu_glUniform1i(Environment& env, std::int32_t location, std::int32_t v0) {
	glUniform1i(location, v0);
}

void emu_glUniform4fv(Environment& env, std::int32_t location, std::uint32_t count, std::uint32_t value_ptr) {
	auto value = env.memory_manager().read_bytes<float>(value_ptr);
	glUniform4fv(location, count, value);
}

void emu_glUniformMatrix4fv(Environment& env, std::int32_t location, std::uint32_t count, bool transpose, std::uint32_t value_ptr) {
	auto value = env.memory_manager().read_bytes<float>(value_ptr);
	glUniformMatrix4fv(location, count, transpose, value);
}

void emu_glUseProgram(Environment& env, std::uint32_t program) {
	glUseProgram(program);
}

void emu_glBindBuffer(Environment& env, std::uint32_t target, std::uint32_t buffer) {
	glBindBuffer(target, buffer);

	spdlog::trace("glBindBuffer(target: {}, buffer: {}) -> {}", target, buffer, glGetError());
}

void emu_glBufferData(Environment& env, std::uint32_t target, std::uint32_t size, std::uint32_t data_ptr, std::uint32_t usage) {
	void* data = nullptr;
	if (data_ptr != 0) {
		data = env.memory_manager().read_bytes<void>(data_ptr);
	}

	glBufferData(target, size, data, usage);

	spdlog::trace("glBufferData(target: {}, size: {}, data: {:#x}, usage: {}) -> {}", target, size, data_ptr, usage, glGetError());
}

void emu_glTexParameteri(Environment& env, std::uint32_t target, std::uint32_t pname, std::int32_t param) {
	glTexParameteri(target, pname, param);
}

void emu_glBindTexture(Environment& env, std::uint32_t target, std::uint32_t texture) {
	glBindTexture(target, texture);

	spdlog::trace("glBindTexture(target: {}, texture: {}) -> {}", target, texture, glGetError());
}

void emu_glActiveTexture(Environment& env, std::uint32_t texture) {
	glActiveTexture(texture);

	spdlog::trace("glActiveTexture(texture: {}) -> {}", texture, glGetError());
}

void emu_glGenTextures(Environment& env, std::uint32_t n, std::uint32_t textures_ptr) {
	std::uint32_t* textures = nullptr;
	if (textures_ptr != 0) {
		textures = env.memory_manager().read_bytes<std::uint32_t>(textures_ptr);
	}

	glGenTextures(n, textures);

	spdlog::trace("glGenTextures(n: {}, textures: {:#x}) -> {}", n, textures_ptr, glGetError());
}

void emu_glDeleteTextures(Environment& env, std::uint32_t n, std::uint32_t textures_ptr) {
	std::uint32_t* textures = nullptr;
	if (textures_ptr != 0) {
		textures = env.memory_manager().read_bytes<std::uint32_t>(textures_ptr);
	}

	glDeleteTextures(n, textures);
}

void emu_glGenBuffers(Environment& env, std::uint32_t n, std::uint32_t buffers_ptr) {
	std::uint32_t* buffers = nullptr;
	if (buffers_ptr != 0) {
		buffers = env.memory_manager().read_bytes<std::uint32_t>(buffers_ptr);
	}

	glGenBuffers(n, buffers);

	spdlog::trace("glGenBuffers(n: {}, buffers: {:#x}) -> {}", n, buffers_ptr, glGetError());
}

void emu_glBlendFunc(Environment& env, std::uint32_t sfactor, std::uint32_t dfactor) {
	glBlendFunc(sfactor, dfactor);
}

void emu_glClearDepthf(Environment& env, float depth) {
	// this function is unavailable on desktop opengl
#ifdef SILENE_USE_EGL
	glClearDepthf(depth);
#else
	glClearDepth(depth);
#endif
}

void emu_glDepthFunc(Environment& env, std::uint32_t func) {
	glDepthFunc(func);
}

void emu_glClearColor(Environment& env, float red, float green, float blue, float alpha) {
	glClearColor(red, green, blue, alpha);
}

void emu_glViewport(Environment& env, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height) {
	glViewport(x, y, width, height);

	spdlog::trace("glViewport(x: {}, y: {}, width: {}, height: {}) -> {}", x, y, width, height, glGetError());
}

void emu_glTexImage2D(Environment& env, std::uint32_t target, std::int32_t level, std::int32_t internalformat, std::uint32_t width, std::uint32_t height, std::int32_t border, std::uint32_t format, std::uint32_t type, std::uint32_t data_ptr) {
	// is this a 2d array?
	void* data = nullptr;
	if (data_ptr != 0) {
		data = env.memory_manager().read_bytes<void>(data_ptr);
	}

	glTexImage2D(target, level, internalformat, width, height, border, format, type, data);

	spdlog::trace("glTexImage2D(target: {}, level: {}, internalformat: {}, width: {}, height: {}, border: {}, format: {}, type: {}, data: {:#x}) -> {}", target, level, internalformat, width, height, border, format, type, data_ptr, glGetError());
}

void emu_glDrawArrays(Environment& env, std::uint32_t mode, std::int32_t first, std::uint32_t count) {
	glDrawArrays(mode, first, count);

	spdlog::trace("glDrawArrays({}, {}, {}) -> {}", mode, first, count, glGetError());
}

void emu_glClear(Environment& env, std::uint32_t mask) {
	glClear(mask);
}

void emu_glDrawElements(Environment& env, std::uint32_t mode, std::uint32_t count, std::uint32_t type, std::uint32_t indices_ptr) {
	void* indices = nullptr;
	if (indices_ptr != 0) {
		indices = env.memory_manager().read_bytes<void>(indices_ptr);
	}

	glDrawElements(mode, count, type, indices);

	spdlog::trace("glDrawElements({}, {}, {}, {:#x}) -> {}", mode, count, type, indices_ptr, glGetError());
}

void emu_glBufferSubData(Environment& env, std::uint32_t target, std::uint32_t offset, std::int32_t size, std::uint32_t data_ptr) {
	void* data = nullptr;
	if (data_ptr != 0) {
		data = env.memory_manager().read_bytes<void>(data_ptr);
	}

	glBufferSubData(target, offset, size, data);

	spdlog::trace("glBufferSubData({}, {}, {}, {:#x}) -> {}", target, offset, size, data_ptr, glGetError());
}

void emu_glVertexAttribPointer(Environment& env, std::uint32_t index, std::int32_t size, std::uint32_t type, bool normalized, std::uint32_t stride, std::uint32_t pointer_ptr) {
	void* pointer = nullptr;
	GLint binding = 0;

	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &binding);

	if (binding) {
		pointer = reinterpret_cast<void*>(pointer_ptr);
	} else if (pointer_ptr != 0) {
		pointer = env.memory_manager().read_bytes<void>(pointer_ptr);
	}

	glVertexAttribPointer(index, size, type, normalized, stride, pointer);

	spdlog::trace("glVertexAttribPointer(index: {}, size: {}, type: {}, normalized: {}, stride: {}, pointer: {:#x}) -> {}", index, size, type, normalized, stride, pointer_ptr, glGetError());
}

void emu_glEnableVertexAttribArray(Environment& env, std::uint32_t index) {
	glEnableVertexAttribArray(index);
	spdlog::trace("glEnableVertexAttribArray(index: {}) -> {}", index, glGetError());
}

void emu_glDisableVertexAttribArray(Environment& env, std::uint32_t index) {
	glDisableVertexAttribArray(index);
	spdlog::trace("glDisableVertexAttribArray(index: {}) -> {}", index, glGetError());
}

void emu_glLineWidth(Environment& env, float width) {
	glLineWidth(width);
}

void emu_glUniform1f(Environment& env, std::int32_t location, float v0) {
	glUniform1f(location, v0);
}

void emu_glBindRenderbuffer(Environment& env, std::uint32_t target, std::uint32_t renderbuffer) {
	glBindRenderbuffer(target, renderbuffer);
}

void emu_glBindFramebuffer(Environment& env, std::uint32_t target, std::uint32_t framebuffer) {
	glBindFramebuffer(target, framebuffer);
}

void emu_glFramebufferTexture2D(Environment& env, std::uint32_t target, std::uint32_t attachment, std::uint32_t textarget, std::uint32_t texture, std::int32_t level) {
	glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

void emu_glGenRenderbuffers(Environment& env, int32_t n, uint32_t renderbuffers_ptr) {
	std::uint32_t* renderbuffers = nullptr;
	if (renderbuffers_ptr != 0) {
		renderbuffers = env.memory_manager().read_bytes<std::uint32_t>(renderbuffers_ptr);
	}

	glGenRenderbuffers(n, renderbuffers);
}

void emu_glFramebufferRenderbuffer(Environment& env, std::uint32_t target, std::uint32_t attachment, std::uint32_t renderbuffertarget, std::int32_t renderbuffer) {
	glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

void emu_glGenFramebuffers(Environment& env, std::int32_t n, std::uint32_t framebuffers_ptr) {
	std::uint32_t* framebuffers = nullptr;
	if (framebuffers_ptr != 0) {
		framebuffers = env.memory_manager().read_bytes<std::uint32_t>(framebuffers_ptr);
	}

	glGenFramebuffers(n, framebuffers);
}

std::uint32_t emu_glCheckFramebufferStatus(Environment& env, std::uint32_t target) {
	return glCheckFramebufferStatus(target);
}

void emu_glUniform4f(Environment& env, std::int32_t location, float v0, GLfloat v1, float v2, float v3) {
	glUniform4f(location, v0, v1, v2, v3);
}

std::uint32_t emu_glGetError(Environment& env) {
	return glGetError();
}

#endif
