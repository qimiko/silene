#include "gdb-server.h"

#include <spdlog/spdlog.h>

#include <numeric>
#include <charconv>
#include <format>
#include <sstream>
#include <iomanip>
#include <bit>
#include <span>

#include <exception>

template <typename T>
T parse_hex(const char* msg, std::uint32_t begin, std::uint32_t end) {
	T value = -1;

	if (begin >= end) {
			return -1;
	}

	if (std::from_chars(
		msg + begin,
		msg + end,
		value, 16
	).ec != std::errc{}) {
		// failed to parse
		return -1;
	}

	return value;
}

std::uint8_t calculate_checksum(const std::string_view& message) {
	return std::accumulate(message.begin(), message.end(), 0) % 256;
}

const char* target_description() {
	return R"(<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target>
	<architecture>arm</architecture>
	<osabi>GNU/Linux</osabi>
	<xi:include href="base-reg.xml"/>
</target>)";
}

const char* reg_description() {
	// source: https://github.com/qemu/qemu/blob/master/gdb-xml/arm-core.xml
	return R"(<?xml version="1.0"?>
<!DOCTYPE feature SYSTEM "gdb-target.dtd">
<feature name="org.gnu.gdb.arm.core">
	<reg name="r0" bitsize="32" />
	<reg name="r1" bitsize="32" />
	<reg name="r2" bitsize="32" />
	<reg name="r3" bitsize="32" />
	<reg name="r4" bitsize="32" />
	<reg name="r5" bitsize="32" />
	<reg name="r6" bitsize="32" />
	<reg name="r7" bitsize="32" />
	<reg name="r8" bitsize="32" />
	<reg name="r9" bitsize="32" />
	<reg name="r10" bitsize="32" />
	<reg name="r11" bitsize="32" />
	<reg name="r12" bitsize="32" />
	<reg name="sp" bitsize="32" type="data_ptr" />
	<reg name="lr" bitsize="32" />
	<reg name="pc" bitsize="32" type="code_ptr" />
	<reg name="cpsr" bitsize="32" regnum="25" />
</feature>)";
}

std::string to_hex_string(const std::span<std::uint8_t>& s) {
	std::stringstream ss;

	for (const auto& c : s) {
		ss << std::setfill('0') << std::setw(2) << std::hex << static_cast<std::uint32_t>(c);
	}

	return ss.str();
}

std::vector<std::uint8_t> from_hex_string(const std::string_view& s) {
	auto data_size = s.length() / 2;
	std::vector<std::uint8_t> data{};

	data.reserve(data_size);

	for (auto i = 0u; i < data_size; i += 2) {
		auto ch = parse_hex<std::uint8_t>(s.data(), i, i + 1);
		data.push_back(ch);
	}

	return data;
}

void GdbServer::begin_connection(const std::string& ip, std::uint16_t port) {
	this->_tcp_server.bind(ip, port);

	spdlog::info("now waiting for debugger to connect to port {}...", port);
	this->_tcp_server.accept();
}

std::optional<std::string_view> GdbServer::read_message(const std::string_view& message) {
		if (message.empty()) {
			return std::nullopt;
		}

		auto begin_pos = message.find('$');
		if (begin_pos == std::string::npos) {
			return std::nullopt;
		}

		auto checksum_pos = message.rfind('#');
		if (checksum_pos == std::string::npos) {
			return std::nullopt;
		}

		auto command = message.substr(begin_pos + 1, checksum_pos - begin_pos - 1);

		auto checksum = parse_hex<std::uint8_t>(message.data(), checksum_pos + 1, message.size());

		// validate checksum
		auto checksum_calc = calculate_checksum(command);

		if (checksum != checksum_calc) {
			return std::nullopt;
		}

		return command;
}

void GdbServer::send_message(const std::string_view& message) {
	std::uint32_t checksum = calculate_checksum(message);

	std::stringstream ss;

	if (!this->_skip_ack) {
		ss << "+";
	}

	ss << "$" << message << "#" << std::setfill('0') << std::setw(2) << std::hex << checksum;

	spdlog::debug("gdb <- {}", ss.str());

	this->_last_message = message;
	this->_tcp_server.write(ss.str());
}

bool GdbServer::dispatch_set(const std::string_view& command) {
	// commands that begin with Q
	if (command == "StartNoAckMode") {
		this->send_message("OK");
		this->_skip_ack = true;

		return true;
	}

	return false;
}

bool GdbServer::dispatch_query(const std::string_view& command) {
	// commands that begin with q
	auto separator_pos = command.find(':');

	auto query = command.substr(0, separator_pos);
	auto data = command.substr(separator_pos + 1);

	// hardcoded messages, don't know if there's a reason to parse beyond this yet
	if (query == "Supported") {
		this->send_message("QStartNoAckMode+;xmlRegisters=arm;swbreak+;hwbreak+;multiprocess+;qXfer:features:read+;qXfer:libraries-svr4:read+;qXfer:exec-file:read+;vContSupported+");
	} else if (query == "C") {
		this->send_message("QC1");
	} else if (query == "HostInfo") {
		// hardcoded fields, triple = arm--linux-android
		this->send_message("triple:61726d2d2d6c696e75782d616e64726f6964;ptrsize:4;endian:little;hostname:73696c656e65;os_version:0.0.1;vm-page-size:4096;");
	} else if (query == "ProcessInfo") {
		this->send_message("pid:1;parent-pid:1;real-uid:1;real-gid:1;effective-uid:1;effective-gid:1;");
	} else if (query == "Attached") {
		this->send_message("1");
	} else if (query == "fThreadInfo") {
		this->send_message("m1");
	} else if (query == "sThreadInfo") {
		this->send_message("l");
	}	else if (query == "Xfer") {
		// maybe it's features:read:target.xml (even though it's probably not)
		// parse this

		// format = qXfer:object:read:annex:offset,length

		auto action_begin = data.find(':');
		auto annex_begin = data.find(':', action_begin + 1);
		auto offset_begin = data.find(':', annex_begin + 1);
		auto length_begin = data.find(',', offset_begin + 1);

		// always forget substr takes lengths
		auto object = data.substr(0, action_begin);
		auto action = data.substr(action_begin + 1, annex_begin - action_begin - 1);
		auto annex = data.substr(annex_begin + 1, offset_begin - annex_begin - 1);
		auto offset = parse_hex<std::uint64_t>(data.data(), offset_begin + 1, length_begin);
		auto length = parse_hex<std::uint64_t>(data.data(), length_begin + 1, data.size());

		if (action != "read") {
			return false;
		}

		if (object == "features") {
			std::string str = "";

			if (annex == "target.xml") {
				str = target_description();
			} else if (annex == "base-reg.xml") {
				str = reg_description();
			} else {
				return false;
			}

			auto data = str.substr(offset, length);

			if (str.size() == data.size() || data.size() == 0) {
				data.insert(0, "l");
			} else {
				data.insert(0, "m");
			}

			this->send_message(data);
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

void GdbServer::continue_exec() {
	this->_last_halt = HaltReason::NoHalt;
}

void GdbServer::step() {
	this->_last_halt = HaltReason::NoHalt;
	this->_halt_on_next = true;
}

bool GdbServer::dispatch_v(const std::string_view& command) {
	// commands that begin with v
	auto separator_pos = command.find(';');

	auto query = command.substr(0, separator_pos);
	auto data = command.substr(separator_pos + 1);

	if (query == "Cont?") {
		this->send_message("vCont;c;s");
	} else if (query == "Cont") {
		if (data.empty()) {
			return false;
		}

		auto action = data.front();
		switch (action) {
			case 'c':
				this->continue_exec();
				break;
			case 's':
				this->step();
				break;
		}
	} else {
		return false;
	}

	return true;
}

bool GdbServer::dispatch_command(const std::string_view& command) {
	if (command.empty()) {
		return false;
	}

	auto type = command.front();
	auto data = command.substr(1);

	switch (command[0]) {
		case 'Q':
			return this->dispatch_set(data);
		case 'q':
			return this->dispatch_query(data);
		case 'v':
			return this->dispatch_v(data);
		case '?':
			this->send_halt_response();
			return true;
		case 'H':
			// sets thread for future operation (but doesn't run, so it's okay)
			this->send_message("OK");
			return true;
		case 'p': {
			// read single register
			if (command.size() < 2) {
				break;
			}

			auto reg = parse_hex<std::uint8_t>(command.data(), 1, command.length());

			auto value = 0u;
			if (reg == 25) {
				value = this->_cpu->Cpsr();
			} else {
				value = this->_cpu->Regs()[reg];
			}

			std::stringstream ss;
			ss << std::setfill('0') << std::setw(8) << std::hex << htonl(value);
			this->send_message(ss.str());

			return true;
		}
		case 'g': {
			// read registers
			std::stringstream ss;
			ss << std::setfill('0') << std::setw(8) << std::hex;

			// usage of htonl is not ideal
			// should the endianness always be big?
			for (const auto& reg : this->_cpu->Regs()) {
				ss << std::setw(8) << htonl(reg);
			}

			ss << std::setw(8) << htonl(this->_cpu->Cpsr());

			this->send_message(ss.str());

			return true;
		}
		case 'm': {
			auto length_begin = command.rfind(',');
			auto offset = parse_hex<std::uint32_t>(command.data(), 1, length_begin);
			auto length = parse_hex<std::uint32_t>(command.data(), length_begin + 1, command.length());

			// determine if read will go out of bounds
			auto edge = static_cast<std::uint64_t>(offset) + length;
			auto memory_max = 0xffff'ffff;
			if (edge > memory_max) {
				length = memory_max - offset;
			}

			auto bytes_begin = this->_memory->read_bytes<std::uint8_t>(offset);
			auto str = to_hex_string({bytes_begin, length});

			this->send_message(str);
			return true;
		}
		/*
		case 'M': {
			auto length_begin = command.rfind(',');
			auto offset = parse_hex<std::uint32_t>(command.data(), 1, length_begin);
			auto length = parse_hex<std::uint32_t>(command.data(), length_begin + 1, command.length());

			// determine if read will go out of bounds
			auto edge = static_cast<std::uint64_t>(offset) + length;
			auto memory_max = 0xffff'ffff;
			if (edge > memory_max) {
				length = memory_max - offset;
			}

			auto bytes_begin = this->_memory->read_bytes<char>(offset);
			auto str = to_hex_string({bytes_begin, length});

			this->send_message("OK");
			return true;
		}
		*/
		case 'D':
			// detach
			this->send_message("OK");
			this->_tcp_server.close();
			return true;
		case 'k':
			// kill connection and let server continue
			this->send_message("X09;process:1");
			this->_tcp_server.close();
			return true;
		case 'C':
		// continue with signal (don't know what that actually implies)
		case 'c':
			// note: the command does support redirecting execution - this doesn't
			this->continue_exec();
			return true;
	}

	return false;
}

void GdbServer::read_message() {
	auto msg = this->_tcp_server.read();

	spdlog::debug("gdb -> {}", msg);

	if (msg == "+") {
		// message was just ack
		return;
	}

	if (msg == "-") {
		// server asked for retransmission
		this->send_message(this->_last_message);
		return;
	}

	auto command_opt = this->read_message(msg);
	if (!command_opt) {
		// something didn't match, ask for a retransmission
		spdlog::warn("invalid message recv: {}", msg);
		this->_tcp_server.write("-");
		return;
	}

	auto command = command_opt.value();

	if (!this->dispatch_command(command)) {
		this->send_message("");
	}
}

void GdbServer::handle_events() {
	// block and wait for new data if server should halt
	if (this->_halt_on_next) {
		this->_halt_on_next = false;
		this->report_halt(HaltReason::Breakpoint);
	}

	while (this->_tcp_server.has_data() || this->_last_halt != HaltReason::NoHalt) {
		if (!this->_tcp_server.connection_alive()) {
			throw std::runtime_error("debugger connection lost");
		}

		this->read_message();
	}
}

void GdbServer::send_halt_response() {
	std::stringstream ss;
	ss << "S" << std::setfill('0') << std::setw(2) << std::hex << static_cast<uint32_t>(this->_last_halt);
	this->send_message(ss.str());
}

void GdbServer::report_halt(HaltReason reason) {
	this->_last_halt = reason;

	if (!this->_tcp_server.connection_alive()) {
		return;
	}

	this->send_halt_response();
}
