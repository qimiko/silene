#pragma once

#ifndef _TCP_SERVER_H
#define _TCP_SERVER_H

#include <cstdint>
#include <string>

/**
 * very basic single connection synchronous tcp server
 */
class TcpServer {
	int _socket_fd{-1};
	int _client_fd{-1};

	static constexpr std::uint32_t READ_BUFFER_SIZE{1024};

public:
	TcpServer();
	~TcpServer();

	void bind(const std::string& ip, std::uint16_t port);
	void accept();

	void close();

	bool has_data();
	std::string read();

	bool connection_alive() const;

	void write(std::string_view data);

	TcpServer(const TcpServer&) = delete;
	TcpServer(TcpServer&&) = delete;
};

#endif
