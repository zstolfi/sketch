#pragma once
#include <cmath>

template <typename T, typename U>
T clamp(T x, U a, U b) {
	return std::max<T>(a, std::min<T>(b, x));
}