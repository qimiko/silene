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

#endif