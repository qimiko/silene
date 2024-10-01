#include "jni.h"

#include "jni/base-robtop-activity.h"

#define REGISTER_STATIC(CLASS, SIGNATURE, NAME) \
	register_static(CLASS, SIGNATURE, &SyscallTranslator::translate_wrap<&jni_##NAME>)

void JniState::pre_init(Environment& env) {
	JavaVM vm;

	vm.ptr_attachCurrentThread = REGISTER_STUB(env, attachCurrentThread);
	vm.ptr_getEnv = REGISTER_STUB(env, getEnv);

	JNIEnv jni_env;

	jni_env.ptr_newStringUTF = REGISTER_STUB(env, newStringUTF);
	jni_env.ptr_getStringUTFChars = REGISTER_STUB(env, getStringUTFChars);
	jni_env.ptr_releaseStringUTFChars = REGISTER_STUB(env, releaseStringUTFChars);
	jni_env.ptr_findClass = REGISTER_STUB(env, findClass);
	jni_env.ptr_deleteLocalRef = REGISTER_STUB(env, deleteLocalRef);
	jni_env.ptr_callStaticVoidMethodV = REGISTER_STUB(env, callStaticVoidMethodV);
	jni_env.ptr_callStaticObjectMethodV = REGISTER_STUB(env, callStaticObjectMethodV);
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

	this->_memory->allocate(sizeof(JavaVM));

	auto env_write_addr = this->_memory->get_next_addr();

	if (env_write_addr & 1) {
		// if the returned pointer is thumbed, increment by 1 to remove it
		env_write_addr++;
		this->_memory->allocate(1);
	}

	jni_env.ptr_self = env_write_addr;

	this->_memory->copy(env_write_addr, &jni_env, sizeof(JNIEnv));
	this->_env_ptr = env_write_addr;

	this->_memory->allocate(sizeof(JNIEnv));

	REGISTER_STATIC("com/customRobTop/BaseRobTopActivity", "getUserID;()Ljava/lang/String;", get_user_id);
}

std::uint32_t JniState::get_vm_ptr() const {
	return this->_vm_ptr;
}

std::uint32_t JniState::get_env_ptr() const {
	return this->_env_ptr;
}

std::uint32_t JniState::emu_newStringUTF(Environment& env, std::uint32_t java_env, std::uint32_t string_ptr) {
	auto str = env.memory_manager()->read_bytes<char>(string_ptr);
	return env.jni().create_string_ref(str);
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

	auto& jni = env.jni();
	if (auto jclass = jni._class_name_mapping.find(class_name); jclass != jni._class_name_mapping.end()) {
		return jclass->second;
	}

	spdlog::warn("FindClass on nonexistent class - {}", class_name);
	return 0;
}

void JniState::emu_deleteLocalRef(Environment& env, std::uint32_t java_env, std::uint32_t local_ref) {
	env.jni().remove_ref(local_ref);
}

void JniState::emu_callStaticVoidMethodV(Environment& env, std::uint32_t java_env, std::uint32_t local_ref, std::uint32_t method_id) {
	env.jni().get_fn(local_ref, method_id)(env);
}

void JniState::emu_callStaticObjectMethodV(Environment& env, std::uint32_t java_env, std::uint32_t local_ref, std::uint32_t method_id) {
	// hopefully, the method we call correctly sets the return type
	env.jni().get_fn(local_ref, method_id)(env);
}

JniState::StaticJavaClass::JniFunction JniState::get_fn(std::uint32_t class_id, std::uint32_t method_id) const {
	auto clazz = _class_mapping.at(class_id);
	return clazz.method_impls.at(method_id);
}

std::uint32_t JniState::emu_getStaticMethodID(Environment& env, std::uint32_t java_env, std::uint32_t class_ptr, std::uint32_t name_ptr, std::uint32_t signature_ptr) {
	auto method_name = env.memory_manager()->read_bytes<char>(name_ptr);
	auto method_signature = env.memory_manager()->read_bytes<char>(signature_ptr);

	auto& jni = env.jni();
	if (auto jclass = jni._class_mapping.find(class_ptr); jclass != jni._class_mapping.end()) {
		auto& clazz = jclass->second;
		auto combined = fmt::format("{};{}", method_name, method_signature);

		if (auto jmethod = clazz.method_names.find(combined); jmethod != clazz.method_names.end()) {
			return jmethod->second;
		}
	}

	spdlog::warn("GetStaticMethodID on nonexistent method - {};{}", method_name, method_signature);
	return 0;
}

void JniState::remove_ref(std::uint32_t vaddr) {
	this->_object_refs.erase(vaddr);
}

std::string JniState::get_ref_value(std::uint32_t vaddr) {
	return this->_object_refs.at(vaddr);
}

std::uint32_t JniState::create_string_ref(const std::string_view& str) {
	auto addr = this->_memory->get_next_word_addr();
	this->_memory->allocate(4);

	// write itself for safekeeping
	this->_memory->write_word(addr, 0xcbfbfbdb);

	this->_object_refs[addr] = std::string(str);

	spdlog::trace("create string ref: {:#08x}", addr);

	return addr;
}

void JniState::emu_releaseStringUTFChars(Environment& env, std::uint32_t java_env, std::uint32_t jstring, std::uint32_t string_ptr) {
	env.libc().free_memory(string_ptr);
	// TODO: should we actually free the jstring here?
}

std::uint32_t JniState::register_static(std::string class_name, std::string signature, StaticJavaClass::JniFunction fn) {
	if (auto jclass = _class_name_mapping.find(class_name); jclass != _class_name_mapping.end()) {
		auto& clazz = _class_mapping[jclass->second];
		if (auto jmethod = clazz.method_names.find(signature); jmethod != clazz.method_names.end()) {
			// oops! we already have a method of that id
			return jmethod->second;
		}

		auto method_id = clazz.method_count;
		clazz.method_count++;

		clazz.method_names[signature] = method_id;
		clazz.method_impls[method_id] = fn;

		return method_id;
	}

	// class doesn't exist, time to create it
	auto class_id = _class_count;
	_class_count++;

	_class_name_mapping[class_name] = class_id;
	_class_mapping[class_id] = {class_id, class_name};

	auto& clazz = _class_mapping[class_id];

	auto method_id = clazz.method_count;
	clazz.method_count++;

	clazz.method_names[signature] = method_id;
	clazz.method_impls[method_id] = fn;

	return method_id;
}

