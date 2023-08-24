#include "jni.h"

void JniState::pre_init(Environment& env) {
	JavaVM vm;

	vm.ptr_attachCurrentThread = REGISTER_STUB(env, attachCurrentThread);
	vm.ptr_getEnv = REGISTER_STUB(env, getEnv);

	JNIEnv jni_env;

	jni_env.ptr_getStringUTFChars = REGISTER_STUB(env, getStringUTFChars);
	jni_env.ptr_releaseStringUTFChars = REGISTER_STUB(env, releaseStringUTFChars);
	jni_env.ptr_findClass = REGISTER_STUB(env, findClass);
	jni_env.ptr_deleteLocalRef = REGISTER_STUB(env, deleteLocalRef);
	jni_env.ptr_callStaticVoidMethodV = REGISTER_STUB(env, callStaticVoidMethodV);
	jni_env.ptr_getStaticMethodID = REGISTER_STUB(env, getStaticMethodID);

	// write after registering symbols so memory is ordered more cleanly

	auto vm_write_addr = this->_memory->get_next_addr();

	if (vm_write_addr & 1) {
		// if the returned pointer is thumbed, increment by 1 to remove it
		vm_write_addr++;
		this->_memory->allocate(1);
	}

	vm.ptr_self = vm_write_addr;

	this->_memory->copy(vm_write_addr, &vm, sizeof(JavaVM));
	this->_vm_ptr = vm_write_addr;

	auto env_write_addr = this->_memory->get_next_addr();

	if (env_write_addr & 1) {
		// if the returned pointer is thumbed, increment by 1 to remove it
		env_write_addr++;
		this->_memory->allocate(1);
	}

	jni_env.ptr_self = env_write_addr;

	this->_memory->copy(env_write_addr, &jni_env, sizeof(JNIEnv));
	this->_env_ptr = env_write_addr;
}

std::uint32_t JniState::get_vm_ptr() const {
	return this->_vm_ptr;
}

std::uint32_t JniState::get_env_ptr() const {
	return this->_env_ptr;
}

std::uint32_t JniState::emu_getStringUTFChars(Environment& env, std::uint32_t java_env, std::uint32_t string, std::uint32_t is_copy_ptr) {
	try {
		auto ref = env.jni().get_ref_value(string);

		// add a null byte
		auto ptr = env.libc().allocate_memory(ref.length() + 1, true);
		env.memory_manager()->copy(ptr, ref.data(), ref.length());

		if (is_copy_ptr != 0) {
			env.memory_manager()->write_byte(is_copy_ptr, 1);
		}

		return ptr;
	} catch (const std::out_of_range& e) {
		spdlog::warn("GetStringUTFChars given invalid value {:#08x}", string);
		return 0;
	}
}

std::uint32_t JniState::emu_getEnv(Environment& env, std::uint32_t java_env, std::uint32_t out_ptr, std::uint32_t version) {
	auto env_ptr = env.jni().get_env_ptr();
	env.memory_manager()->write_word(out_ptr, env_ptr);

	return 0;
}

std::uint32_t JniState::emu_attachCurrentThread(Environment& env, std::uint32_t java_env, std::uint32_t p_env_ptr, std::uint32_t attach_args_ptr) {
	// "Trying to attach a thread that is already attached is a no-op."
	return 0;
}

std::uint32_t JniState::emu_findClass(Environment& env, std::uint32_t java_env, std::uint32_t name_ptr) {
	auto class_name = env.memory_manager()->read_bytes<char>(name_ptr);
	spdlog::info("TODO: FindClass - {}", class_name);

	return 0;
}

void JniState::emu_deleteLocalRef(Environment& env, std::uint32_t java_env, std::uint32_t local_ref) {
	env.jni().remove_ref(local_ref);
}

void JniState::emu_callStaticVoidMethodV(Environment& env, std::uint32_t java_env, std::uint32_t local_ref, std::uint32_t method_id) {
	spdlog::info("TODO: CallStaticVoidMethodV");
}

std::uint32_t JniState::emu_getStaticMethodID(Environment& env, std::uint32_t java_env, std::uint32_t class_ptr, std::uint32_t name_ptr, std::uint32_t signature_ptr) {
	auto method_name = env.memory_manager()->read_bytes<char>(name_ptr);
	auto method_signature = env.memory_manager()->read_bytes<char>(signature_ptr);

	spdlog::info("TODO: GetStaticMethodID - {};{}", method_name, method_signature);

	return 0;
}

void JniState::remove_ref(std::uint32_t vaddr) {
	this->_object_refs.erase(vaddr);
}

std::string& JniState::get_ref_value(std::uint32_t vaddr) {
	return this->_object_refs.at(vaddr);
}

std::uint32_t JniState::create_string_ref(const std::string_view& str) {
	auto addr = this->_memory->get_next_word_addr();
	this->_memory->allocate(4);

	// write itself for safekeeping
	this->_memory->write_word(addr, 0xcbfbfbdb);

	this->_object_refs[addr] = std::string(str);

	spdlog::info("create string ref: {:#08x}", addr);

	return addr;
}

void JniState::emu_releaseStringUTFChars(Environment& env, std::uint32_t java_env, std::uint32_t jstring, std::uint32_t string_ptr) {
	env.libc().free_memory(string_ptr);
	// TODO: should we actually free the jstring here?
}
