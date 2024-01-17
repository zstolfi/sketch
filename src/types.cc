#pragma once
#include "types.hh"

const std::map<std::string_view,ElementType> elementTypeFromString {
	{"Data"  , ElementType::Data  },
	{"Pencil", ElementType::Pencil},
	{"Brush" , ElementType::Brush },
	{"Fill"  , ElementType::Fill  },
};

// RawSketch Sketch::flatten() {
// 	return RawSketch {.strokes = {
// 		{{0,0}, {800,0}, {800,600}, {0,600}, {0,0}}
// 	}};
// }


// Raw Data Types:
RawPoint::operator Point() const {
	return Point {.x=x, .y=y, .pressure=1.0};
}

RawStroke::operator Stroke() const {
	Stroke s {};
	s.points.reserve(points.size());
	for (RawPoint p : points)
		s.points.push_back(static_cast<Point>(p));
	s.diameter = 3;
	return s;
}

RawSketch::operator Sketch() const {
	Sketch s {};
	for (RawStroke t : strokes)
		s.atoms.push_back(static_cast<Stroke>(t));
	// s.elements = {
	// 	Element {
	// 		ElementType::Data,
	// 		std::span { s.atoms },
	// 		.modifiers = {}
	// 	};
	// };
	return s;
}



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



std::ostream& operator<<(std::ostream& os, const Point& p) {
	os << "(" << p.x << ", "  << p.y << ", "  << p.pressure << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Stroke& s) {
	os << "width: " << s.diameter << "\n";
	os << "points:";
	for (Point p : s.points)
		os << "\t" << p;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Marker& m) {
	os << "message: " << m.text;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Sketch& s) {
	auto groupIt = s.elements.begin();

	for (auto it=s.atoms.begin(); it!=s.atoms.end(); ++it) {
		if (std::holds_alternative<Stroke>(*it))
			os << std::get<Stroke>(*it);
		else if (std::holds_alternative<Marker>(*it))
			os << std::get<Marker>(*it);
		/* ... */

		if (std::distance(it, s.atoms.end())) os << "\n";
	}
	return os;
}