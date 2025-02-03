#pragma once

#ifndef _GDB_SERVER_H
#define _GDB_SERVER_H

#include <optional>
#include <memory>
#include <unordered_map>

#include <dynarmic/interface/A32/a32.h>

#include "paged-memory.hpp"

#include "tcp-server.h"

class GdbServer {
public:
	enum class HaltReason {
		NoHalt = 0,
		Hangup = 1,
		Interrupt = 2,
		Quit = 3,
		IllegalInstruction = 4,
		Breakpoint = 5,
		Abort = 6,
		EmulationTrap = 7,
		Kill = 9,
		SegmentationFault = 11,

		SwBreak = 99
	};

private:
	TcpServer _tcp_server{};

	std::string _last_message{};

	bool _skip_ack{false};
	HaltReason _last_halt{HaltReason::Breakpoint};
	bool _halt_on_next{false};

	std::unordered_map<std::uint32_t, std::uint32_t> _sw_breakpoints{};

	PagedMemory& _memory;
	std::shared_ptr<Dynarmic::A32::Jit> _cpu{nullptr};

	void send_halt_response();

	/**
	 * reads a message from the tcp stream
	 * if there was an invalid message, then no value is returned
	 */
	std::optional<std::string_view> read_message(std::string_view message);

	void send_message(std::string_view data);

	bool dispatch_query(std::string_view command);
	bool dispatch_set(std::string_view command);
	bool dispatch_v(std::string_view command);

	bool dispatch_command(std::string_view command);

	void read_message();

	void continue_exec();
	void step();

public:

	GdbServer(PagedMemory& memory, std::shared_ptr<Dynarmic::A32::Jit> cpu) : _memory{memory}, _cpu{cpu} {}

	/**
	 * creates the server and blocks execution until a client is connected
	 */
	void begin_connection(const std::string& ip, std::uint16_t port);

	/**
	 * handles incoming events (if there are any)
	 * will block if the debugger says to halt
	 */
	void handle_events();

	void report_halt(HaltReason reason);
};

#endif