#pragma once

#ifndef _JNI_H
#define _JNI_H

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <string>
#include <vector>
#include <string_view>

class StateHolder;
class Environment;
class PagedMemory;

namespace Silene {

struct JNIEnv {
	std::uint32_t ptr_self{0}; // should point to itself
	// TODO: provide some way to default initialize that doesn't rely on hardcoding
	std::uint32_t ptr_defaultFn{0x21};
	std::uint32_t reserved1{0};
	std::uint32_t reserved2{0};

	// half of these aren't called by GD/cocos
	std::uint32_t ptr_getVersion{ptr_defaultFn}; // (JNIEnv*) -> jint
	std::uint32_t ptr_defineClass{ptr_defaultFn}; // (JNIEnv*, const char*, jobject, const jbyte*, jsize) -> jclass
	std::uint32_t ptr_findClass{ptr_defaultFn}; // (JNIEnv*, const char*) -> jclass
	std::uint32_t ptr_fromReflectedMethod{ptr_defaultFn}; // (JNIEnv*, jobject) -> jmethodID
	std::uint32_t ptr_fromReflectedField{ptr_defaultFn}; // (JNIEnv*, jobject) -> jfieldID
	std::uint32_t ptr_toReflectedMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jboolean) -> jobject
	std::uint32_t ptr_getSuperClass{ptr_defaultFn}; // (JNIEnv*, jclass) -> jclass
	std::uint32_t ptr_isAssignableFrom{ptr_defaultFn}; // (JNIEnv*, jclass, jclass) -> jboolean
	std::uint32_t ptr_toReflectedField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jboolean) -> jobject
	std::uint32_t ptr_throw{ptr_defaultFn}; // (JNIEnv*, jthrowable) -> jint
	std::uint32_t ptr_throwNew{ptr_defaultFn}; // (JNIEnv *, jclass, const char *) -> jint
	std::uint32_t ptr_exceptionOccurred{ptr_defaultFn}; // (JNIEnv*) -> jthrowable
	std::uint32_t ptr_exceptionDescribe{ptr_defaultFn}; // (JNIEnv*) -> void
	std::uint32_t ptr_exceptionClear{ptr_defaultFn}; // (JNIEnv*) -> void
	std::uint32_t ptr_fatalError{ptr_defaultFn}; // (JNIEnv*, const char*) -> void
	std::uint32_t ptr_pushLocalFrame{ptr_defaultFn}; // (JNIEnv*, jint) -> jint
	std::uint32_t ptr_popLocalFrame{ptr_defaultFn}; // (JNIEnv*, jobject) -> jobject
	std::uint32_t ptr_newGlobalRef{ptr_defaultFn}; // (JNIEnv*, jobject) -> jobject
	std::uint32_t ptr_deleteGlobalRef{ptr_defaultFn}; // (JNIEnv*, jobject) -> void
	std::uint32_t ptr_deleteLocalRef{ptr_defaultFn}; // (JNIEnv*, jobject) -> void
	std::uint32_t ptr_isSameObject{ptr_defaultFn}; // (JNIEnv*, jobject, jobject) -> jboolean
	std::uint32_t ptr_newLocalRef{ptr_defaultFn}; // (JNIEnv*, jobject) -> jobject
	std::uint32_t ptr_ensureLocalCapacity{ptr_defaultFn}; // (JNIEnv*, jint) -> jint
	std::uint32_t ptr_allocObject{ptr_defaultFn}; // (JNIEnv*, jclass) -> jobject
	std::uint32_t ptr_newObject{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jobject
	std::uint32_t ptr_newObjectV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jobject
	std::uint32_t ptr_newObjectA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jobject
	std::uint32_t ptr_getObjectClass{ptr_defaultFn}; // (JNIEnv*, jobject) -> jclass
	std::uint32_t ptr_isInstanceOf{ptr_defaultFn}; // (JNIEnv*, jobject, jclass) -> jboolean
	std::uint32_t ptr_getMethodID{ptr_defaultFn}; // (JNIEnv*, jclass, const char*, const char*) -> jmethodID
	std::uint32_t ptr_callObjectMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jobject
	std::uint32_t ptr_callObjectMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jobject
	std::uint32_t ptr_callObjectMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jobject
	std::uint32_t ptr_callBooleanMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jboolean
	std::uint32_t ptr_callBooleanMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jboolean
	std::uint32_t ptr_callBooleanMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jboolean
	std::uint32_t ptr_callByteMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jbyte
	std::uint32_t ptr_callByteMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jbyte
	std::uint32_t ptr_callByteMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jbyte
	std::uint32_t ptr_callCharMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jchar
	std::uint32_t ptr_callCharMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jchar
	std::uint32_t ptr_callCharMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jchar
	std::uint32_t ptr_callShortMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jshort
	std::uint32_t ptr_callShortMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jshort
	std::uint32_t ptr_callShortMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jshort
	std::uint32_t ptr_callIntMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jint
	std::uint32_t ptr_callIntMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jint
	std::uint32_t ptr_callIntMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jint
	std::uint32_t ptr_callLongMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jlong
	std::uint32_t ptr_callLongMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jlong
	std::uint32_t ptr_callLongMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jlong
	std::uint32_t ptr_callFloatMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jfloat
	std::uint32_t ptr_callFloatMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jfloat
	std::uint32_t ptr_callFloatMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jfloat
	std::uint32_t ptr_callDoubleMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> jdouble
	std::uint32_t ptr_callDoubleMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> jdouble
	std::uint32_t ptr_callDoubleMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> jdouble
	std::uint32_t ptr_callVoidMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, ...) -> void
	std::uint32_t ptr_callVoidMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, va_list) -> void
	std::uint32_t ptr_callVoidMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jmethodID, jvalue*) -> void
	std::uint32_t ptr_callNonvirtualObjectMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jobject
	std::uint32_t ptr_callNonvirtualObjectMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jobject
	std::uint32_t ptr_callNonvirtualObjectMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jobject
	std::uint32_t ptr_callNonvirtualBooleanMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jboolean
	std::uint32_t ptr_callNonvirtualBooleanMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jboolean
	std::uint32_t ptr_callNonvirtualBooleanMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jboolean
	std::uint32_t ptr_callNonvirtualByteMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jbyte
	std::uint32_t ptr_callNonvirtualByteMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jbyte
	std::uint32_t ptr_callNonvirtualByteMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jbyte
	std::uint32_t ptr_callNonvirtualCharMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jchar
	std::uint32_t ptr_callNonvirtualCharMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jchar
	std::uint32_t ptr_callNonvirtualCharMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jchar
	std::uint32_t ptr_callNonvirtualShortMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jshort
	std::uint32_t ptr_callNonvirtualShortMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jshort
	std::uint32_t ptr_callNonvirtualShortMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jshort
	std::uint32_t ptr_callNonvirtualIntMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jint
	std::uint32_t ptr_callNonvirtualIntMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jint
	std::uint32_t ptr_callNonvirtualIntMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jint
	std::uint32_t ptr_callNonvirtualLongMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jlong
	std::uint32_t ptr_callNonvirtualLongMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jlong
	std::uint32_t ptr_callNonvirtualLongMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jlong
	std::uint32_t ptr_callNonvirtualFloatMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jfloat
	std::uint32_t ptr_callNonvirtualFloatMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jfloat
	std::uint32_t ptr_callNonvirtualFloatMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jfloat
	std::uint32_t ptr_callNonvirtualDoubleMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> jdouble
	std::uint32_t ptr_callNonvirtualDoubleMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> jdouble
	std::uint32_t ptr_callNonvirtualDoubleMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> jdouble
	std::uint32_t ptr_callNonvirtualVoidMethod{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, ...) -> void
	std::uint32_t ptr_callNonvirtualVoidMethodV{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, va_list) -> void
	std::uint32_t ptr_callNonvirtualVoidMethodA{ptr_defaultFn}; // (JNIEnv*, jobject, jclass, jmethodID, jvalue*) -> void
	std::uint32_t ptr_getFieldID{ptr_defaultFn}; // (JNIEnv*, jclass, const char*, const char*) -> jfieldID
	std::uint32_t ptr_getObjectField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jobject
	std::uint32_t ptr_getBooleanField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jboolean
	std::uint32_t ptr_getByteField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jbyte
	std::uint32_t ptr_getCharField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jchar
	std::uint32_t ptr_getShortField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jshort
	std::uint32_t ptr_getIntField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jint
	std::uint32_t ptr_getLongField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jlong
	std::uint32_t ptr_getFloatField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jfloat
	std::uint32_t ptr_getDoubleField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID) -> jdouble
	std::uint32_t ptr_setObjectField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jobject) -> void
	std::uint32_t ptr_setBooleanField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jboolean) -> void
	std::uint32_t ptr_setByteField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jbyte) -> void
	std::uint32_t ptr_setCharField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jchar) -> void
	std::uint32_t ptr_setShortField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jshort) -> void
	std::uint32_t ptr_setIntField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jint) -> void
	std::uint32_t ptr_setLongField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jlong) -> void
	std::uint32_t ptr_setFloatField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jfloat) -> void
	std::uint32_t ptr_setDoubleField{ptr_defaultFn}; // (JNIEnv*, jobject, jfieldID, jdouble) -> void
	std::uint32_t ptr_getStaticMethodID{ptr_defaultFn}; // (JNIEnv*, jclass, const char*, const char*) -> jmethodID
	std::uint32_t ptr_callStaticObjectMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jobject
	std::uint32_t ptr_callStaticObjectMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jobject
	std::uint32_t ptr_callStaticObjectMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jobject
	std::uint32_t ptr_callStaticBooleanMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jboolean
	std::uint32_t ptr_callStaticBooleanMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jboolean
	std::uint32_t ptr_callStaticBooleanMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jboolean
	std::uint32_t ptr_callStaticByteMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jbyte
	std::uint32_t ptr_callStaticByteMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jbyte
	std::uint32_t ptr_callStaticByteMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jbyte
	std::uint32_t ptr_callStaticCharMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jchar
	std::uint32_t ptr_callStaticCharMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jchar
	std::uint32_t ptr_callStaticCharMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jchar
	std::uint32_t ptr_callStaticShortMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jshort
	std::uint32_t ptr_callStaticShortMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jshort
	std::uint32_t ptr_callStaticShortMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jshort
	std::uint32_t ptr_callStaticIntMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jint
	std::uint32_t ptr_callStaticIntMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jint
	std::uint32_t ptr_callStaticIntMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jint
	std::uint32_t ptr_callStaticLongMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jlong
	std::uint32_t ptr_callStaticLongMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jlong
	std::uint32_t ptr_callStaticLongMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jlong
	std::uint32_t ptr_callStaticFloatMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jfloat
	std::uint32_t ptr_callStaticFloatMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jfloat
	std::uint32_t ptr_callStaticFloatMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jfloat
	std::uint32_t ptr_callStaticDoubleMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> jdouble
	std::uint32_t ptr_callStaticDoubleMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> jdouble
	std::uint32_t ptr_callStaticDoubleMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> jdouble
	std::uint32_t ptr_callStaticVoidMethod{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, ...) -> void
	std::uint32_t ptr_callStaticVoidMethodV{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, va_list) -> void
	std::uint32_t ptr_callStaticVoidMethodA{ptr_defaultFn}; // (JNIEnv*, jclass, jmethodID, jvalue*) -> void
	std::uint32_t ptr_getStaticFieldID{ptr_defaultFn}; // (JNIEnv*, jclass, const char*, const char*) -> jfieldID
	std::uint32_t ptr_getStaticObjectField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jobject
	std::uint32_t ptr_getStaticBooleanField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jboolean
	std::uint32_t ptr_getStaticByteField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jbyte
	std::uint32_t ptr_getStaticCharField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jchar
	std::uint32_t ptr_getStaticShortField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jshort
	std::uint32_t ptr_getStaticIntField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jint
	std::uint32_t ptr_getStaticLongField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jlong
	std::uint32_t ptr_getStaticFloatField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jfloat
	std::uint32_t ptr_getStaticDoubleField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID) -> jdouble
	std::uint32_t ptr_setStaticObjectField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jobject) -> void
	std::uint32_t ptr_setStaticBooleanField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jboolean) -> void
	std::uint32_t ptr_setStaticByteField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jbyte) -> void
	std::uint32_t ptr_setStaticCharField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jchar) -> void
	std::uint32_t ptr_setStaticShortField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jshort) -> void
	std::uint32_t ptr_setStaticIntField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jint) -> void
	std::uint32_t ptr_setStaticLongField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jlong) -> void
	std::uint32_t ptr_setStaticFloatField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jfloat) -> void
	std::uint32_t ptr_setStaticDoubleField{ptr_defaultFn}; // (JNIEnv*, jclass, jfieldID, jdouble) -> void
	std::uint32_t ptr_newString{ptr_defaultFn}; // (JNIEnv*, const jchar*, jsize) -> jstring
	std::uint32_t ptr_getStringLength{ptr_defaultFn}; // (JNIEnv*, jstring) -> jsize
	std::uint32_t ptr_getStringChars{ptr_defaultFn}; // (JNIEnv*, jstring, jboolean*) -> const jchar*
	std::uint32_t ptr_releaseStringChars{ptr_defaultFn}; // (JNIEnv*, jstring, const jchar*) -> void
	std::uint32_t ptr_newStringUTF{ptr_defaultFn}; // (JNIEnv*, const char*) -> jstring
	std::uint32_t ptr_getStringUTFLength{ptr_defaultFn}; // (JNIEnv*, jstring) -> jsize
	std::uint32_t ptr_getStringUTFChars{ptr_defaultFn}; // (JNIEnv*, jstring, jboolean*) -> const char*
	std::uint32_t ptr_releaseStringUTFChars{ptr_defaultFn}; // (JNIEnv*, jstring, const char*) -> void
	std::uint32_t ptr_getArrayLength{ptr_defaultFn}; // (JNIEnv*, jarray) -> jsize
	std::uint32_t ptr_newObjectArray{ptr_defaultFn}; // (JNIEnv*, jsize, jclass, jobject) -> jobjectArray
	std::uint32_t ptr_getObjectArrayElement{ptr_defaultFn}; // (JNIEnv*, jobjectArray, jsize) -> jobject
	std::uint32_t ptr_setObjectArrayElement{ptr_defaultFn}; // (JNIEnv*, jobjectArray, jsize, jobject) -> void
	std::uint32_t ptr_newBooleanArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jbooleanArray
	std::uint32_t ptr_newByteArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jbyteArray
	std::uint32_t ptr_newCharArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jcharArray
	std::uint32_t ptr_newShortArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jshortArray
	std::uint32_t ptr_newIntArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jintArray
	std::uint32_t ptr_newLongArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jlongArray
	std::uint32_t ptr_newFloatArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jfloatArray
	std::uint32_t ptr_newDoubleArray{ptr_defaultFn}; // (JNIEnv*, jsize) -> jdoubleArray
	std::uint32_t ptr_getBooleanArrayElements{ptr_defaultFn}; // (JNIEnv*, jbooleanArray, jboolean*) -> jboolean*
	std::uint32_t ptr_getByteArrayElements{ptr_defaultFn}; // (JNIEnv*, jbyteArray, jboolean*) -> jbyte*
	std::uint32_t ptr_getCharArrayElements{ptr_defaultFn}; // (JNIEnv*, jcharArray, jboolean*) -> jchar*
	std::uint32_t ptr_getShortArrayElements{ptr_defaultFn}; // (JNIEnv*, jshortArray, jboolean*) -> jshort*
	std::uint32_t ptr_getIntArrayElements{ptr_defaultFn}; // (JNIEnv*, jintArray, jboolean*) -> jint*
	std::uint32_t ptr_getLongArrayElements{ptr_defaultFn}; // (JNIEnv*, jlongArray, jboolean*) -> jlong*
	std::uint32_t ptr_getFloatArrayElements{ptr_defaultFn}; // (JNIEnv*, jfloatArray, jboolean*) -> jfloat*
	std::uint32_t ptr_getDoubleArrayElements{ptr_defaultFn}; // (JNIEnv*, jdoubleArray, jboolean*) -> jdouble*
	std::uint32_t ptr_releaseBooleanArrayElements{ptr_defaultFn}; // (JNIEnv*, jbooleanArray, jboolean*, jint) -> void
	std::uint32_t ptr_releaseByteArrayElements{ptr_defaultFn}; // (JNIEnv*, jbyteArray, jbyte*, jint) -> void
	std::uint32_t ptr_releaseCharArrayElements{ptr_defaultFn}; // (JNIEnv*, jcharArray, jchar*, jint) -> void
	std::uint32_t ptr_releaseShortArrayElements{ptr_defaultFn}; // (JNIEnv*, jshortArray, jshort*, jint) -> void
	std::uint32_t ptr_releaseIntArrayElements{ptr_defaultFn}; // (JNIEnv*, jintArray, jint*, jint) -> void
	std::uint32_t ptr_releaseLongArrayElements{ptr_defaultFn}; // (JNIEnv*, jlongArray, jlong*, jint) -> void
	std::uint32_t ptr_releaseFloatArrayElements{ptr_defaultFn}; // (JNIEnv*, jfloatArray, jfloat*, jint) -> void
	std::uint32_t ptr_releaseDoubleArrayElements{ptr_defaultFn}; // (JNIEnv*, jdoubleArray, jdouble*, jint) -> void
	std::uint32_t ptr_getBooleanArrayRegion{ptr_defaultFn}; // (JNIEnv*, jbooleanArray, jsize, jsize, jboolean*) -> void
	std::uint32_t ptr_getByteArrayRegion{ptr_defaultFn}; // (JNIEnv*, jbyteArray, jsize, jsize, jbyte*) -> void
	std::uint32_t ptr_getCharArrayRegion{ptr_defaultFn}; // (JNIEnv*, jcharArray, jsize, jsize, jchar*) -> void
	std::uint32_t ptr_getShortArrayRegion{ptr_defaultFn}; // (JNIEnv*, jshortArray, jsize, jsize, jshort*) -> void
	std::uint32_t ptr_getIntArrayRegion{ptr_defaultFn}; // (JNIEnv*, jintArray, jsize, jsize, jint*) -> void
	std::uint32_t ptr_getLongArrayRegion{ptr_defaultFn}; // (JNIEnv*, jlongArray, jsize, jsize, jlong*) -> void
	std::uint32_t ptr_getFloatArrayRegion{ptr_defaultFn}; // (JNIEnv*, jfloatArray, jsize, jsize, jfloat*) -> void
	std::uint32_t ptr_getDoubleArrayRegion{ptr_defaultFn}; // (JNIEnv*, jdoubleArray, jsize, jsize, jdouble*) -> void
	std::uint32_t ptr_setBooleanArrayRegion{ptr_defaultFn}; // (JNIEnv*, jbooleanArray, jsize, jsize, const jboolean*) -> void
	std::uint32_t ptr_setByteArrayRegion{ptr_defaultFn}; // (JNIEnv*, jbyteArray, jsize, jsize, const jbyte*) -> void
	std::uint32_t ptr_setCharArrayRegion{ptr_defaultFn}; // (JNIEnv*, jcharArray, jsize, jsize, const jchar*) -> void
	std::uint32_t ptr_setShortArrayRegion{ptr_defaultFn}; // (JNIEnv*, jshortArray, jsize, jsize, const jshort*) -> void
	std::uint32_t ptr_setIntArrayRegion{ptr_defaultFn}; // (JNIEnv*, jintArray, jsize, jsize, const jint*) -> void
	std::uint32_t ptr_setLongArrayRegion{ptr_defaultFn}; // (JNIEnv*, jlongArray, jsize, jsize, const jlong*) -> void
	std::uint32_t ptr_setFloatArrayRegion{ptr_defaultFn}; // (JNIEnv*, jfloatArray, jsize, jsize, const jfloat*) -> void
	std::uint32_t ptr_setDoubleArrayRegion{ptr_defaultFn}; // (JNIEnv*, jdoubleArray, jsize, jsize, const jdouble*) -> void
	std::uint32_t ptr_registerNatives{ptr_defaultFn}; // (JNIEnv*, jclass, const JNINativeMethod*, jint) -> jint
	std::uint32_t ptr_unregisterNatives{ptr_defaultFn}; // (JNIEnv*, jclass) -> jint
	std::uint32_t ptr_monitorEnter{ptr_defaultFn}; // (JNIEnv*, jobject) -> jint
	std::uint32_t ptr_monitorExit{ptr_defaultFn}; // (JNIEnv*, jobject) -> jint
	std::uint32_t ptr_getJavaVM{ptr_defaultFn}; // (JNIEnv*, JavaVM**) -> jint
	std::uint32_t ptr_getStringRegion{ptr_defaultFn}; // (JNIEnv*, jstring, jsize, jsize, jchar*) -> void
	std::uint32_t ptr_getStringUTFRegion{ptr_defaultFn}; // (JNIEnv*, jstring, jsize, jsize, char*) -> void
	std::uint32_t ptr_getPrimitiveArrayCritical{ptr_defaultFn}; // (JNIEnv*, jarray, jboolean*) -> void*
	std::uint32_t ptr_releasePrimitiveArrayCritical{ptr_defaultFn}; // (JNIEnv*, jarray, void*, jint) -> void
	std::uint32_t ptr_getStringCritical{ptr_defaultFn}; // (JNIEnv*, jstring, jboolean*) -> const jchar*
	std::uint32_t ptr_releaseStringCritical{ptr_defaultFn}; // (JNIEnv*, jstring, const jchar*) -> void
	std::uint32_t ptr_newWeakGlobalRef{ptr_defaultFn}; // (JNIEnv*, jobject) -> jweak
	std::uint32_t ptr_deleteWeakGlobalRef{ptr_defaultFn}; // (JNIEnv*, jweak) -> void
	std::uint32_t ptr_exceptionCheck{ptr_defaultFn}; // (JNIEnv*) -> jboolean
	std::uint32_t ptr_newDirectByteBuffer{ptr_defaultFn}; // (JNIEnv*, void*, jlong) -> jobject
	std::uint32_t ptr_getDirectBufferAddress{ptr_defaultFn}; // (JNIEnv*, jobject) -> void*
	std::uint32_t ptr_getDirectBufferCapacity{ptr_defaultFn}; // (JNIEnv*, jobject) -> jlong
	std::uint32_t ptr_getObjectRefType{ptr_defaultFn}; // (JNIEnv*, jobject) -> jobjectRefType
};

struct JavaVM {
	std::uint32_t ptr_self{0};
	std::uint32_t ptr_defaultFn{0x21};
	std::uint32_t reserved{0};

	std::uint32_t ptr_destroyJavaVM{ptr_defaultFn}; // (JavaVM*) -> jint
	std::uint32_t ptr_attachCurrentThread{ptr_defaultFn}; // (JavaVM*, JNIEnv**, void*) -> jint
	std::uint32_t ptr_detachCurrentThread{ptr_defaultFn}; // (JavaVM*) -> jint
	std::uint32_t ptr_getEnv{ptr_defaultFn}; // (JavaVM*, void**, jint) -> jint
	std::uint32_t ptr_attachCurrentThreadAsDaemon{ptr_defaultFn}; // (JavaVM*, JNIEnv**, void*) -> jint
};

class JniState {
	/**
	 * GD doesn't use non-static classes for anything,
	 * so there's not much reason to implement proper classes
	 */
	struct StaticJavaClass {
		using JniFunction = void(*)(Environment& env);

		std::uint32_t id;
		std::string name;

		std::unordered_map<std::string /* method_signature */, std::uint32_t /* method_id */> method_names{};
		std::unordered_map<std::uint32_t /* method_id */, JniFunction> method_impls{};
		std::uint32_t method_count{1};
	};

	PagedMemory& _memory;

	std::uint32_t _vm_ptr{0};
	std::uint32_t _env_ptr{0};

	using RefType = std::variant<std::string, std::vector<int>, std::vector<float>>;

	std::unordered_map<std::uint32_t /* vaddr */, RefType /* stored */> _object_refs{};

	// use a non-zero value to avoid tricking it
	std::uint32_t _class_count{1};
	std::unordered_map<std::string /* class_name */, std::uint32_t /* class_id */> _class_name_mapping{};
	std::unordered_map<std::uint32_t /* class_id */, StaticJavaClass /* class */> _class_mapping{};

	static std::uint32_t emu_newStringUTF(Environment& env, std::uint32_t java_env, std::uint32_t string_ptr);
	static std::uint32_t emu_getStringUTFChars(Environment& env, std::uint32_t java_env, std::uint32_t string, std::uint32_t is_copy_ptr);
	static void emu_releaseStringUTFChars(Environment& env, std::uint32_t java_env, std::uint32_t jstring, std::uint32_t string_ptr);
	static void emu_deleteLocalRef(Environment& env, std::uint32_t java_env, std::uint32_t local_ref);

	static std::uint32_t emu_getArrayLength(Environment& env, std::uint32_t java_env, std::uint32_t jarray);
	static void emu_getIntArrayRegion(Environment& env, std::uint32_t java_env, std::uint32_t jarray, std::uint32_t start, std::uint32_t len, std::uint32_t buf_ptr);
	static void emu_getFloatArrayRegion(Environment& env, std::uint32_t java_env, std::uint32_t jarray, std::uint32_t start, std::uint32_t len, std::uint32_t buf_ptr);

	static std::uint32_t emu_getEnv(Environment& env, std::uint32_t java_env, std::uint32_t out_ptr, std::uint32_t version);
	static std::uint32_t emu_attachCurrentThread(Environment& env, std::uint32_t java_env, std::uint32_t p_env_ptr, std::uint32_t attach_args_ptr);

	static std::uint32_t emu_findClass(Environment& env, std::uint32_t java_env, std::uint32_t name_ptr);
	static std::uint32_t emu_getStaticMethodID(Environment& env, std::uint32_t java_env, std::uint32_t class_ptr, std::uint32_t name_ptr, std::uint32_t signature_ptr);

	static void emu_callStaticMethodV(Environment& env, std::uint32_t java_env, std::uint32_t local_ref, std::uint32_t method_id);

	StaticJavaClass::JniFunction get_fn(std::uint32_t class_id, std::uint32_t method_id) const;

public:
	void pre_init(const StateHolder& env);

	inline std::uint32_t get_vm_ptr() const {
		return this->_vm_ptr;
	}

	inline std::uint32_t get_env_ptr() const {
		return this->_env_ptr;
	}

	/**
	 * removes a stored reference
	 */
	void remove_ref(std::uint32_t vaddr);

	/**
	 * gets the value associated with a stored reference
	 */
	RefType& get_ref_value(std::uint32_t vaddr);

	/**
	 * stores a reference to a string variable, like a jstring
	 */
	std::uint32_t create_string_ref(std::string_view str);

	/**
	 * Stores a reference to a generic variable
	 */
	std::uint32_t create_ref(RefType t);

	/**
	 * registers a static jni method
	 */
	std::uint32_t register_static(std::string class_name, std::string signature, StaticJavaClass::JniFunction fn);

	JniState(PagedMemory& memory) : _memory(memory) {}
};

};

#endif
