#pragma once
#include "modifiers.hh"

Affine::Affine() : m{1,0,0 , 0,1,0 , 0,0,1} {}
Affine::Affine(std::array<float,9> m) : m{m} {}

std::vector<Atom> Affine::operator()(std::span<const Atom> atoms) {
	std::vector<Atom> result {};
	ranges::copy(atoms, result.end());
	for (Atom& a : result) {
		assert(std::holds_alternative<Stroke>(a));
		Stroke& stroke = std::get<Stroke>(a);
		// TODO: Stroke scaling for non Pencil elements
		// stroke.diameter *= scaleFactor
		for (Point& p : stroke.points) {
			p = Point {
				.x = int16_t(p.x*m[0] + p.y*m[1] + m[2]),
				.y = int16_t(p.x*m[3] + p.y*m[4] + m[5]),
				.pressure = p.pressure
			};
		}
	}
	return result;
}

std::vector<Atom> Array::operator()(std::span<const Atom> atoms) {
	std::vector<Atom> result {};
	/* ... */
	return result;
}

namespace {
	using Fn_Arg = std::span<const Atom>;
	using Fn_Ret = std::vector<Atom>;

	template <typename... Ts>
	concept ModCallable = requires {
		( (std::is_invocable_r_v<Fn_Ret, Ts, Fn_Arg>) && ... );
	};

	static_assert(ModCallable<Array, Affine>);
}