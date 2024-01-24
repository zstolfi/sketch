#include "types.hh"
#include "base36.hh"
#include "util.hh"
#include <algorithm>
#include <cassert>

// void RawSketch::sendTo(std::ostream& os) {
// 	for (const RawStroke& s : strokes) {
// 		os << " ";
// 		for (const RawPoint& p : s.points) {
// 			os << Base36::toString<2,unsigned>(p.x)
// 			   << Base36::toString<2,unsigned>(p.y);
// 		}
// 	}
// }

/* ~~ .HSC Data Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Point::Point(int x, int y, float p)
: x{x}, y{y}, pressure{p} {}

Point Point::fromRaw(const RawPoint& p) {
	return Point {p.x, p.y, 1.0};
}

Stroke::Stroke() {}

Stroke::Stroke(unsigned d, std::vector<Point> p)
: diameter{d}, points{p} {}

Stroke Stroke::fromRaw(const RawStroke& s) {
	Stroke self {3, {}};
	self.points.reserve(s.points.size());
	for (const RawPoint& p : s.points) {
		self.points.push_back(Point::fromRaw(p));
	}
	return self;
}

Sketch::Sketch() {}

Sketch::Sketch(std::vector<Element> e)
: elements{e} {}

Sketch Sketch::fromRaw(const RawSketch& s) {
	Sketch self {};
	self.elements = {
		Element {ElementType::Data, {}}
	};
	self.elements[0].atoms.reserve(s.strokes.size());
	for (const RawStroke& t : s.strokes) {
		self.elements[0].atoms.push_back(Stroke::fromRaw(t));
	}
	return self;
}

Element::Element(
	ElementType t, std::vector<Atom> a
)
: type{t}, atoms{a} {}

Element::Element(
	ElementType t, std::vector<Atom> a, std::vector<Modifier> m
)
: type{t}, atoms{a}, modifiers{m} {}

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Mod::Affine::Affine() : m{1,0,0 , 0,1,0 , 0,0,1} {}
Mod::Affine::Affine(float x) : m{x,0,0 , 0,x,0 , 0,0,x} {}
Mod::Affine::Affine(std::array<float,9> m) : m{m} {}

constexpr auto Mod::Affine::operator*(Affine other) -> Affine {
	const auto& a = this->m, b = other.m;
	return Affine ({
		// Sudoku solution for 1/22/2024:
		a[0]*b[0] + a[1]*b[3] + a[2]*b[6],
		a[0]*b[1] + a[1]*b[4] + a[2]*b[7],
		a[0]*b[2] + a[1]*b[5] + a[2]*b[8],

		a[3]*b[0] + a[4]*b[3] + a[5]*b[6],
		a[3]*b[1] + a[4]*b[4] + a[5]*b[7],
		a[3]*b[2] + a[4]*b[5] + a[5]*b[8],

		a[6]*b[0] + a[7]*b[3] + a[8]*b[6],
		a[6]*b[1] + a[7]*b[4] + a[8]*b[7],
		a[6]*b[2] + a[7]*b[5] + a[8]*b[8],
	});
}

constexpr auto Mod::Affine::operator*(Point p) -> Point {
	return Point {
		.x = m[0]*p.x + m[1]*p.y + m[2]*1.0,
		.y = m[3]*p.x + m[4]*p.y + m[5]*1.0
		/* .pressure */ 1.0,
	};
}

auto Mod::Affine::operator()(std::span<const Atom> atoms) const {
	std::vector<Atom> result {};
	result.reserve(atoms.size());
	assert(ranges::all_of(atoms, std::holds_alternative<Atom_t>));

	auto multiplyP = [this](Point p) -> Point {
		return (*this) * p;
	}

	auto multiplyS = [this](const Atom_t& s) -> Atom_t {
		return Atom_t {
			.diameter = s.diameter;
			.points {s.points | views::transform(multiplyP)}
		};
	}

	ranges::copy(
		atoms | views::transform(multiplyS),
		std::back_inserter(result)
	);

	return result;
}

Mod::Array::Array(Affine tf, std::size_t n) : transformation{tf}, n{n} {}

auto Mod::Array::operator()(std::span<const Atom> atoms) const {
	std::vector<Atom> result {};
	result.reserve(n * atoms.size());

	auto applyTf = [this](const Atom_t& s) -> Atom_t {
		return Util::pow(transformation, i)(s);
	}

	for (std::size_t i=0; i<n; i++) {
		ranges::copy(
			atoms | views::transform(applyTf)
			std::back_inserter(result);
		);
	}

	return result;
}

Mod::Uppercase::Uppercase() {}

auto Mod::Uppercase::operator()(std::span<const Atom> atoms) const {
	// std::vector<Atom> result (atoms.size(), Atom_t {});
	std::vector<Atom> result {};
	result.reserve(atoms.size());
	assert(ranges::all_of(atoms, std::holds_alternative<Atom_t>));

	auto toUpperM = [](const Atom_t& m) -> Atom_t {
		return Atom_t {
			.text {m.text | views::transform(std::toUpper)}
		};
	}

	ranges::copy(
		atoms | views::transform(toUpperM),
		std::back_inserter(result)
	);

	return result;
}

/* ~~ Main "Rendering" Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

RawSketch Sketch::flatten() {
	return RawSketch ({
		/* ... */
		RawStroke ({{0,0}, {800,0}, {800,600}, {0,600}, {0,0}})
	});
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


std::ostream& operator<<(std::ostream& os, const RawPoint& p) {
	return os << Base36::toString<2,unsigned>(p.x)
	/*     */ << Base36::toString<2,unsigned>(p.y);
}

std::ostream& operator<<(std::ostream& os, const RawStroke& stroke) {
	for (const RawPoint& p : stroke.points) os << p;
	return os;
}

std::ostream& operator<<(std::ostream& os, const RawSketch& sketch) {
	for (const RawStroke& s : sketch.strokes) os << " " << s;
	return os;
}



std::ostream& operator<<(std::ostream& os, const Point& p) {
	os << "(" << p.x << ", "  << p.y << ", "  << p.pressure << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Stroke& s) {
	os << "width: " << s.diameter << "\t";
	os << "points:";
	for (Point p : s.points) os << "\t" << p;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Marker& m) {
	os << "message: " << m.text;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Sketch& s) {
	for (const Element& elem : s.elements) {
		std::cout << "ElementTypeID: " << (int)elem.type << "\n";
		for (const Atom& atom : elem.atoms) {
			std::visit(
				[&](const auto& a) { os << "\t" << a << "\n"; },
				atom
			);
		}
		os << "\n";
	}
	return os;
}