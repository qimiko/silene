#include <cstdint>
#include <array>
#include <variant>

#include <dynarmic/interface/A32/coprocessor.h>

// this is necessary for libc! sorry

class AndroidCP15 final : public Dynarmic::A32::Coprocessor {
private:
	std::uint32_t _thread_id{0x00c0ffee};

	using CoprocReg = Dynarmic::A32::CoprocReg;

	std::optional<Callback> CompileInternalOperation(bool two, std::uint32_t opc1, CoprocReg CRd, CoprocReg CRn, CoprocReg CRm, std::uint32_t opc2) override;
	CallbackOrAccessOneWord CompileSendOneWord(bool two, std::uint32_t opc1, CoprocReg CRn, CoprocReg CRm, std::uint32_t opc2) override;
	CallbackOrAccessTwoWords CompileSendTwoWords(bool two, std::uint32_t opc, CoprocReg CRm) override;
	CallbackOrAccessOneWord CompileGetOneWord(bool two, std::uint32_t opc1, CoprocReg CRn, CoprocReg CRm, std::uint32_t opc2) override;
	CallbackOrAccessTwoWords CompileGetTwoWords(bool two, std::uint32_t opc, CoprocReg CRm) override;
	std::optional<Callback> CompileLoadWords(bool two, bool long_transfer, CoprocReg CRd, std::optional<std::uint8_t> option) override;
	std::optional<Callback> CompileStoreWords(bool two, bool long_transfer, CoprocReg CRd, std::optional<std::uint8_t> option) override;
};