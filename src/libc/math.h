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

float emu_sqrtf(Environment& env, float x) {
  return std::sqrtf(x);
}

double emu_ceil(Environment& env, double x) {
  return std::ceil(x);
}

float emu_ceilf(Environment& env, float x) {
  return std::ceilf(x);
}

float emu_roundf(Environment& env, float x) {
  return std::roundf(x);
}

#endif