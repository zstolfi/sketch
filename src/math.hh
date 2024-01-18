#pragma once
#include <algorithm>
#include <cmath>
#include <cassert>

using Real = float;
struct Vec2 { Real x, y; };

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

template <typename... Ts>
Real min(Ts... args) {
	return (sizeof...(Ts) > 0)
		? std::min({(Real)args...})
		: std::numeric_limits<Real>::max();
}

template <typename... Ts>
Real max(Ts... args) {
	return (sizeof...(Ts) > 0)
		? std::max({(Real)args...})
		: std::numeric_limits<Real>::min();
}

Real clamp(Real x, Real a, Real b);

template <typename T>
constexpr T pow(T x, std::size_t y) {
	T result {1};
	while (y--) result *= x;
	return result;
}

constexpr auto Log2_36 = 5.1699;
constexpr auto Log2_10 = 3.3219;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Real dot (Vec2 a, Vec2 b);
Real dot2(Vec2 a)        ;
Real len (Vec2 a)        ;
Vec2 norm(Vec2 a)        ;

Real SDFline(Vec2 p, Vec2 a, Vec2 b);