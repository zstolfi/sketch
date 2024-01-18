#include "types.hh"
#include <algorithm>
#include <iostream>

RawSketch Sketch::flatten() {
	return RawSketch ({
		RawStroke ({{0,0}, {800,0}, {800,600}, {0,600}, {0,0}})
	});
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