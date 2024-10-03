#include "zip-file.h"

ZipFile::ZipFile() {
	_zip_handle = mz_zip_reader_create();
}

ZipFile::ZipFile(const std::string& filename) : ZipFile() {
	auto err = mz_zip_reader_open_file_in_memory(_zip_handle, filename.c_str());
	if (err) {
		spdlog::warn("failed to open zip file: {}", err);
	}
}

ZipFile::~ZipFile() {
	if (_zip_handle != nullptr) {
		mz_zip_reader_close(_zip_handle);
		mz_zip_reader_delete(&_zip_handle);
	}
}

bool ZipFile::has_file(const std::string& path) {
	auto err = mz_zip_reader_locate_entry(_zip_handle, path.c_str(), false);
	if (err) {
		return false;
	}

	return true;
}

std::vector<std::uint8_t> ZipFile::read_file_bytes(const std::string& path) {
	auto err = mz_zip_reader_locate_entry(_zip_handle, path.c_str(), false);
	if (err) {
		return {};
	}

	std::vector<std::uint8_t> r{};

	auto len = mz_zip_reader_entry_save_buffer_length(_zip_handle);
	r.reserve(len);

	mz_zip_reader_entry_save_buffer(_zip_handle, r.data(), len);

	return r;
}
