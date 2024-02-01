#include "types.hh"
#include "base36.hh"
#include "util.hh"
#include <algorithm>

/* ~~ Points & Strokes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Sketch::Sketch() {}
Sketch::Sketch(std::vector<Element> v)
: elements{v} {}

FlatSketch::FlatSketch() {}
FlatSketch::FlatSketch(std::vector<Atom::FlatStroke> v)
: strokes{v} {}

Atom::Stroke::Stroke()
: diameter{3} {}
Atom::Stroke::Stroke(unsigned d, std::vector<Point> v)
: diameter{d}, points{v} {}

Atom::Stroke::Point::Point(int x, int y, double p)
: x{x}, y{y}, pressure{p} {}

Atom::FlatStroke::FlatStroke() {}
Atom::FlatStroke::FlatStroke(std::vector<Point> v)
: points{v} {}

Atom::FlatStroke::Point::Point(int x, int y)
: x{x}, y{y} {}

/* ~~ Up-Cast Operators ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Atom::FlatStroke::operator Stroke() {
	auto toStrokePoint = [](Atom::FlatStroke::Point flat) {
		return Atom::Stroke::Point {flat.x, flat.y, 1.0};
	};

	return Stroke {3,
		points
		| views::transform(toStrokePoint)
		| ranges::to<std::vector>()
	};
}

FlatSketch::operator Sketch() {
	return Sketch ({ Data {strokes} });
}

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Mod::Affine::Affine() : matrix{1,0,0 , 0,1,0 , 0,0,1} {}
Mod::Affine::Affine(double x) : matrix{x,0,0 , 0,x,0 , 0,0,x} {}
Mod::Affine::Affine(std::array<double,9> m) : matrix{m} {}

auto Mod::Affine::operator*(Affine other) const -> Affine {
	const auto& a = this->matrix, b = other.matrix;
	return Affine (std::array {
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

auto Mod::Affine::operator()(std::span<const Atom::Stroke> strokes)
const -> std::vector<Atom::Stroke> {
	auto multiplyP = [&m = this->matrix](Atom::Stroke::Point p) {
		return Atom::Stroke::Point {
			int(m[0]*p.x + m[1]*p.y + m[2]),
			int(m[3]*p.x + m[4]*p.y + m[5]),
			/* .pressure */ 1.0,
		};
	};

	auto multiplyS = [&](const Atom::Stroke& s) -> Atom::Stroke {
		return Atom::Stroke {
			s.diameter,
			s.points
			| views::transform(multiplyP)
			| ranges::to<std::vector>(),
		};
	};

	return strokes
		| views::transform(multiplyS)
		| ranges::to<std::vector>();
}

Mod::Array::Array(std::size_t n, Affine tf) : N{n}, transformation{tf} {}

auto Mod::Array::operator()(std::span<const Atom::Stroke> strokes)
const -> std::vector<Atom::Stroke> {
	auto applyMatPower = [this, &strokes](std::size_t i) {
		return Util::pow(transformation, i)(strokes);
	};

	return views::iota(0uz, N)
		| views::transform(applyMatPower)
		| views::join // Labeled under "experimental-library".
		| ranges::to<std::vector>();
}

Mod::Uppercase::Uppercase() {}

auto Mod::Uppercase::operator()(std::span<const Atom::Marker> markers)
const -> std::vector<Atom::Marker> {
	auto toUpperM = [&](const Atom::Marker& m) {
		return Atom::Marker {
			m.text
			| views::transform(Util::toUpper)
			| ranges::to<std::string>()
		};
	};

	return markers
		| views::transform(toUpperM)
		| ranges::to<std::vector>();
}

/* ~~ Print Sketch ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(std::ostream& os, const Sketch& sketch) {
	for (std::size_t i = sketch.elements.size()
	;    const Element& e : sketch.elements) {
		os << e << (--i ? ",\n" : "");
	}
	return os << ";";
}

std::ostream& operator<<(std::ostream& os, const Element& element) {
	std::visit(Util::Overloaded {
		[&os](const Brush&)  { os << "Brush " ; },
		[&os](const Pencil&) { os << "Pencil "; },
		[&os](const Data&)   { os << "Data "  ; },
		[&os](const Marker&) { os << "Marker "; },
	}, element);

	std::visit([&os](const auto& e) {
		os << e.atoms << e.modifiers;
	}, element);

	return os;
}

/* ~~ Print Atoms ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(std::ostream& os, const StrokeAtoms& v) {
	os << "[ ";
	for (Atom::Stroke s : v) {
		os << Base36::toString<2,unsigned>(s.diameter) << " ";
		for (std::size_t i = s.points.size()
		;    const auto& p : s.points) {
			os << Base36::toString<3,signed>(p.x)
			   << Base36::toString<3,signed>(p.y)
			   << "\'";
			unsigned pressure = (36*36-1) * p.pressure;
			os << Base36::toString<2,unsigned>(pressure)
			   << (--i ? "\'" : "");
		}
	}
	return os << " ]";
}

std::ostream& operator<<(std::ostream& os, const FlatStrokeAtoms& v) {
	os << "[ ";
	for (Atom::FlatStroke s : v) {
		for (std::size_t i = s.points.size()
		;    const auto& p : s.points) {
			os << Base36::toString<3,signed>(p.x)
			   << Base36::toString<3,signed>(p.y)
			   << (--i ? "\'" : "");
		}
	}
	return os << " ]";
}

std::ostream& operator<<(std::ostream& os, const MarkerAtom& m) {
	return os << "(" << m.text << ")";
}

/* ~~ Print Modifiers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(std::ostream& os, const StrokeModifiers v) {
	for (const Mod::Of_Stroke& mod : v) {
		os << "\n\t";
		std::visit(Util::Overloaded {
			[&os](const Mod::Affine& a) {
				os << "Affine [ ";
				for (std::size_t i=0; i<9; i++) {
					os << a.matrix[i] << " ";
				}
				os << "]";
			},
			[&os](const Mod::Array& a) {
				os << "Array [ ";
				os << a.N << " ";
				for (std::size_t i=0; i<9; i++) {
					os << a.transformation.matrix[i] << " ";
				}
				os << "]";
			},
		}, mod);
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const MarkerModifiers v) {
	for (const Mod::Of_Marker& mod : v) {
		os << "\n\t";
		std::visit(Util::Overloaded {
			[&os](const Mod::Uppercase&) {
				os << "Uppercase [ ]";
			},
		}, mod);
	}
	return os;
}