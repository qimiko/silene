#include "android-coprocessor.hpp"

#include <spdlog/spdlog.h>

// this is just for thread local storage, which we don't currently use, so most of these can do nothing

std::optional<AndroidCP15::Callback> AndroidCP15::CompileInternalOperation(bool two, std::uint32_t opc1, CoprocReg CRd, CoprocReg CRn, CoprocReg CRm, std::uint32_t opc2) {
	return std::nullopt;
}

AndroidCP15::CallbackOrAccessOneWord AndroidCP15::CompileSendOneWord(bool two, std::uint32_t opc1, CoprocReg CRn, CoprocReg CRm, std::uint32_t opc2) {
	return CallbackOrAccessOneWord{};
}

AndroidCP15::CallbackOrAccessTwoWords AndroidCP15::CompileSendTwoWords(bool two, std::uint32_t opc, CoprocReg CRm) {
	return CallbackOrAccessTwoWords{};
}

AndroidCP15::CallbackOrAccessOneWord AndroidCP15::CompileGetOneWord(bool two, std::uint32_t opc1, CoprocReg CRn, CoprocReg CRm, std::uint32_t opc2) {
	if (opc1 == 0 && CRn == CoprocReg::C13 && CRm == CoprocReg::C0 && opc2 == 3) {
		// whatever a TPIDRURO (read only thread id) is
		return &_thread_id;
	}

	return CallbackOrAccessOneWord{};
}

AndroidCP15::CallbackOrAccessTwoWords AndroidCP15::CompileGetTwoWords(bool two, std::uint32_t opc, CoprocReg CRm) {
	return CallbackOrAccessTwoWords{};
}

std::optional<AndroidCP15::Callback> AndroidCP15::CompileLoadWords(bool two, bool long_transfer, CoprocReg CRd, std::optional<std::uint8_t> option) {
	return std::nullopt;
}

std::optional<AndroidCP15::Callback> AndroidCP15::CompileStoreWords(bool two, bool long_transfer, CoprocReg CRd, std::optional<std::uint8_t> option) {
	return std::nullopt;
}
