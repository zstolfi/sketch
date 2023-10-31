#pragma once
#include <cmath>
#include <cassert>

using Real = float;
struct Vec2 { Real x, y; };

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

template <typename... Ts> Real
min(Ts... args) { return std::min({(Real)args...}); }

Real min() { return std::numeric_limits<Real>::max(); }

template <typename... Ts> Real
max(Ts... args) { return std::max({(Real)args...}); }

Real max() { return std::numeric_limits<Real>::min(); }

Real clamp(Real x, Real a, Real b) {
	assert(a <= b);
	return max(a, min(b, x));
}

template <typename T>
constexpr T pow(T x, std::size_t y) {
	T result {1};
	while (y--) result *= x;
	return result;
}

constexpr auto Log2_36 = 5.1699;
constexpr auto Log2_10 = 3.3219;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Real dot (Vec2 a, Vec2 b) { return a.x*b.x + a.y*b.y; }
Real dot2(Vec2 a)         { return dot(a,a); }
Real len (Vec2 a)         { return sqrt(dot2(a)); }
Vec2 norm(Vec2 a)         { return {a.x/len(a), a.y/len(a)}; }


Real SDFline(Vec2 p, Vec2 a, Vec2 b) {
	Vec2 c = {p.x-a.x, p.y-a.y};
	Vec2 d = {b.x-a.x, b.y-a.y};
	// Point case
	if (dot2(d) == 0)
		return len(c);
	Real h = clamp(dot(c,d)/dot2(d), 0, 1);
	return len({c.x - d.x*h, c.y - d.y*h});
}