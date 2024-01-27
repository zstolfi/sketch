#pragma once
#include <algorithm>
#include <cmath>
#include <cassert>

using Real = double;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

template <typename... Ts>
constexpr Real min(Ts... args) {
	return (sizeof...(Ts) > 0)
		? std::min({(Real)args...})
		: std::numeric_limits<Real>::max();
}

template <typename... Ts>
constexpr Real max(Ts... args) {
	return (sizeof...(Ts) > 0)
		? std::max({(Real)args...})
		: std::numeric_limits<Real>::min();
}

constexpr Real clamp(Real x, Real a, Real b) {
	assert(a <= b);
	return max(a, min(b, x));
}