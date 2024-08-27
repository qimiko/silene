#include "tcp-server.h"

#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include <spdlog/spdlog.h>

#include <sstream>
#include <system_error>

TcpServer::TcpServer() {
	this->_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_socket_fd < 0) {
		throw std::system_error(errno, std::generic_category(), "TcpServer: failed to create socket");
	}

	// allow reuse of the port immediately (instead of the annoying wait)
	auto value = 1;
	if (setsockopt(this->_socket_fd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) == -1) {
		throw std::system_error(errno, std::generic_category(), "TcpServer: failed to set REUSEPORT");
	}

	if (setsockopt(this->_socket_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1) {
		throw std::system_error(errno, std::generic_category(), "TcpServer: failed to set REUSEADDR");
	}
}

void TcpServer::bind(const std::string& ip, std::uint16_t port) {
	in_addr ip_addr{};
	if (!inet_aton(ip.data(), &ip_addr)) {
		throw std::runtime_error("bind: failed to parse ip");
	}

	// can't initialize the struct inline as it defines extra things
	sockaddr_in addr{};

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = ip_addr;

	if (::bind(this->_socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
		throw std::system_error(errno, std::generic_category(), "bind: failed to bind socket");
	}

	if (::listen(this->_socket_fd, 1) != 0) {
		throw std::system_error(errno, std::generic_category(), "bind: failed to listen");
	}
}

void TcpServer::accept() {
	this->_client_fd = ::accept(this->_socket_fd, nullptr, nullptr);
	if (this->_client_fd < 0) {
		throw std::system_error(errno, std::generic_category(), "accept");
	}
}

void TcpServer::write(const std::string_view& data) {
	if (this->_client_fd == -1) {
		return;
	}

	if (::write(this->_client_fd, data.data(), data.length()) == -1) {
		throw std::system_error(errno, std::generic_category(), "write");
	}
}

bool TcpServer::has_data() {
	if (this->_client_fd == -1) {
		return false;
	}

	pollfd fds{};
	fds.fd = this->_client_fd;
	fds.events = POLLIN;

	auto events_received = poll(&fds, 1, 0);
	if (events_received == -1) {
		throw std::system_error(errno, std::generic_category(), "has_data: failed to poll");
	}

	if (events_received <= 0) {
		return false;
	}

	if (fds.revents & POLLHUP) {
		this->close();
	}

	if (fds.revents & POLLERR) {
		this->close();
	}

	if (fds.revents & POLLNVAL) {
		return false;
	}

	if (fds.revents & POLLIN) {
		return true;
	}

	return false;
}

void TcpServer::close() {
	spdlog::warn("client connection has been closed");

	::close(this->_client_fd);
	this->_client_fd = -1;
}

bool TcpServer::connection_alive() const {
	return this->_client_fd != -1 && this->_socket_fd != -1;
}

std::string TcpServer::read() {
	if (this->_client_fd == -1) {
		return "";
	}

	std::stringstream ss;

	std::int32_t last_bytes_read = 0u;

	do {
		char data[READ_BUFFER_SIZE] = {};

		last_bytes_read = recv(this->_client_fd, &data, sizeof(data), 0);
		if (last_bytes_read < 0) {
			throw std::system_error(errno, std::generic_category(), "read: failed to recv");
		}

		if (last_bytes_read == 0) {
			this->close();
		}

		ss << data;
	} while (last_bytes_read == READ_BUFFER_SIZE);

	return ss.str();
}

TcpServer::~TcpServer() {
	if (this->_client_fd >= 0) {
		::close(this->_client_fd);
	}

	if (this->_socket_fd >= 0) {
		::close(this->_socket_fd);
	}
}
