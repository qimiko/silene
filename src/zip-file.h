#pragma once

#ifndef _ZIP_FILE_H
#define _ZIP_FILE_H

#include <string>

#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

#include <spdlog/spdlog.h>

class ZipFile {
	void* _zip_handle{nullptr};

public:
	ZipFile();
	ZipFile(const std::string& filename);

	bool has_file(const std::string& path);
	std::vector<std::uint8_t> read_file_bytes(const std::string& path);

	~ZipFile();
};

#endif
