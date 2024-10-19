#pragma once

#ifndef _LIBC_MATH_H
#define _LIBC_MATH_H

#include <cmath>

#include "../environment.h"

double emu_sin(Environment& env, double x) {
  return std::sin(x);
}

float emu_sinf(Environment& env, float x) {
  return std::sinf(x);
}

double emu_cos(Environment& env, double x) {
  return std::cos(x);
}

float emu_cosf(Environment& env, float x) {
  return std::cosf(x);
}

double emu_sqrt(Environment& env, double x) {
  return std::sqrt(x);
}

float emu_sqrtf(Environment& env, float x) {
  return std::sqrtf(x);
}

double emu_ceil(Environment& env, double x) {
  return std::ceil(x);
}

float emu_ceilf(Environment& env, float x) {
  return std::ceilf(x);
}

double emu_round(Environment& env, double x) {
  return std::round(x);
}

float emu_roundf(Environment& env, float x) {
  return std::roundf(x);
}

std::int32_t emu_lroundf(Environment& env, float x) {
  return static_cast<std::int32_t>(std::lroundf(x));
}

double emu_floor(Environment& env, double x) {
  return std::floor(x);
}

float emu_floorf(Environment& env, float x) {
  return std::floorf(x);
}

float emu_powf(Environment& env, float base, float exp) {
  return std::powf(base, exp);
}

double emu_pow(Environment& env, double base, double exp) {
  return std::pow(base, exp);
}

double emu_atan(Environment& env, double x) {
  return std::atan(x);
}

double emu_atan2(Environment& env, double y, double x) {
  return std::atan2(y, x);
}

float emu_atan2f(Environment& env, float y, float x) {
  return std::atan2f(y, x);
}

double emu_acos(Environment& env, double x) {
  return std::acos(x);
}

float emu_acosf(Environment& env, float x) {
  return std::acosf(x);
}

float emu_fmodf(Environment& env, float x, float y) {
  return std::fmodf(x, y);
}

double emu_fmod(Environment& env, double x, double y) {
  return std::fmod(x, y);
}

float emu_asinf(Environment& env, float x) {
  return std::asinf(x);
}

double emu_asinh(Environment& env, double x) {
  return std::asinh(x);
}

float emu_asinhf(Environment& env, float x) {
  return std::asinhf(x);
}

float emu_tanf(Environment& env, float x) {
  return std::tanf(x);
}

double emu_tanh(Environment& env, double x) {
  return std::tanh(x);
}

float emu_tanhf(Environment& env, float x) {
  return std::tanhf(x);
}

int emu___fpclassifyd(Environment& env, double x) {
  return std::fpclassify(x);
}

#endif