#include "types.hh"
// #include "base36.hh"
#include <algorithm>
#include <ranges>
#include <cassert>
namespace ranges = std::ranges;

void RawSketch::sendTo(std::ostream& os) {
	// for (const RawStroke& s : strokes) {
	// 	os << " ";
	// 	for (const RawPoint& p : s.points) {
	// 		os << asBase36<2>(p.x) << asBase36(p.y);
	// 	}
	// }
}

RawSketch Sketch::flatten() {
	return RawSketch ({
		RawStroke ({{0,0}, {800,0}, {800,600}, {0,600}, {0,0}})
	});
}

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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Mod::Affine::Affine() : m{1,0,0 , 0,1,0 , 0,0,1} {}
Mod::Affine::Affine(std::array<float,9> m) : m{m} {}

std::vector<Atom> Mod::Affine::operator()(std::span<const Atom> atoms) {
	std::vector<Atom> result {};
	ranges::copy(atoms, result.end());
	for (Atom& a : result) {
		assert(std::holds_alternative<Stroke>(a));
		Stroke& stroke = std::get<Stroke>(a);
		// TODO: Stroke scaling for non Pencil elements
		// stroke.diameter *= scaleFactor
		for (Point& p : stroke.points) {
			p.x = p.x*m[0] + p.y*m[1] + m[2];
			p.y = p.x*m[3] + p.y*m[4] + m[5];
			p.pressure = p.pressure;
		}
	}
	return result;
}

Mod::Array::Array(Affine tf, std::size_t n) : transformation{tf}, n{n} {}

std::vector<Atom> Mod::Array::operator()(std::span<const Atom> atoms) {
	std::vector<Atom> result {};
	/* ... */
	return result;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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