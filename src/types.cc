#include "types.hh"
#include "base36.hh"
#include "util.hh"
#include <algorithm>

/* ~~ Points & Strokes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Point::Point(int x, int y, double p)
: x{x}, y{y}, pressure{p} {}

Stroke::Stroke()
: diameter{3} {}

Stroke::Stroke(unsigned d, std::vector<Point> v)
: diameter{d}, points{v} {}

/* ~~ Flattened Points & Strokes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

FlatPoint::FlatPoint(int x, int y)
: Point{x, y, 1.0} {}

FlatPoint FlatPoint::fromRaw(const RawPoint& raw)
{ return FlatPoint {raw.x, raw.y}; }

FlatStroke::FlatStroke()
: Stroke{} {}

FlatStroke::FlatStroke(std::vector<FlatPoint> v)
: Stroke{3, {}}
{ points.reserve(v.size()); for (FlatPoint p : v) points.push_back(p); }

FlatStroke FlatStroke::fromRaw(const RawStroke& raw) {
	return FlatStroke {
		raw.points
		| views::transform(FlatPoint::fromRaw)
		| ranges::to<std::vector>()
	};
}



Sketch::Sketch() {}

Sketch::Sketch(std::vector<Element> e)
: elements{e} {}

Sketch Sketch::fromRaw(const RawSketch& raw) {
	return Sketch {
		std::vector { Element {
			Element::Data,
			raw.strokes
			| views::transform(FlatStroke::fromRaw)
			| ranges::to<std::vector>()
		} }
	};
}

Element::Element(Element::Type t, Atoms a)
: type{t}, atoms{a} {}

Element::Element(Element::Type t, Atoms a, Modifiers m)
: type{t}, atoms{a}, modifiers{m} {}

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

auto Mod::Affine::operator*(Point p) const -> Point {
	return Point {
		int(matrix[0]*p.x + matrix[1]*p.y + matrix[2]),
		int(matrix[3]*p.x + matrix[4]*p.y + matrix[5]),
		/* .pressure */ 1.0,
	};
}

auto Mod::Affine::operator()(std::span<const Stroke> strokes) const
-> std::vector<Stroke> {
	auto multiplyP = [this](Point p) {
		return (*this) * p;
	};

	auto multiplyS = [&](const Stroke& s) -> Stroke {
		return Stroke {
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

auto Mod::Array::operator()(std::span<const Stroke> strokes) const
-> std::vector<Stroke> {
	auto applyMatPower = [this, &strokes](std::size_t i) {
		return Util::pow(transformation, i)(strokes);
	};

	return views::iota(0uz, N)
		| views::transform(applyMatPower)
		| views::join // Labeled under "experimental-library".
		| ranges::to<std::vector>();
}

Mod::Uppercase::Uppercase() {}

auto Mod::Uppercase::operator()(std::span<const Marker> markers) const
-> std::vector<Marker> {
	auto toUpperM = [&](const Marker& m) {
		return Marker {
			m.text
			| views::transform(Util::toUpper)
			| ranges::to<std::string>()
		};
	};

	return markers
		| views::transform(toUpperM)
		| ranges::to<std::vector>();
}

/* ~~ Main "Flatten" Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto Sketch::render() -> RawSketch {
	return RawSketch ({
		/* TODO ... */
		RawStroke ({{0,0}, {800,0}, {800,600}, {0,600}, {0,0}})
	});
}

/* ~~ Print Raw ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(std::ostream& os, const RawSketch& sketch) {
	for (const RawStroke& s : sketch.strokes) {
		os << " ";
		for (const RawPoint& p : s.points) {
			os << Base36::toString<2,unsigned>(p.x)
			   << Base36::toString<2,unsigned>(p.y);
		}
	}
	return os;
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
	switch (element.type) {
		case Element::Brush : os << "Brush " ; break;
		case Element::Data  : os << "Data "  ; break;
		case Element::Marker: os << "Marker "; break;
		default: std::unreachable();
	}

	std::visit(Util::Overloaded {
		[&os](const std::vector<Stroke>& strokes) {
			os << "[ ";
			for (const Stroke& s : strokes) {
				os << s << " ";
			}
			os << "]";
		},
		[&os](const std::vector<FlatStroke>& raw) {
			os << "[ ";
			for (const FlatStroke& s : raw) {
				os << s << " ";
			}
			os << "]";
		},
		[&os](const Marker& m) {
			os << m;
		},
	}, element.atoms);

	std::visit([&os](const auto& modifiers) {
		for (const auto& mod : modifiers) {
			std::visit([&os](const auto& m) {
				os << "\n\t" << m;
			}, mod);
		}
	}, element.modifiers);

	return os;
}

/* ~~ Print Atoms ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(std::ostream& os, const Stroke& s) {
	os << Base36::toString<2,unsigned>(s.diameter) << " ";
	for (std::size_t i = s.points.size()
	;    const Point& p : s.points) {
		os << Base36::toString<3,signed>(p.x)
		   << Base36::toString<3,signed>(p.y)
		   << "\'";
		unsigned pressure = (36*36-1) * p.pressure;
		os << Base36::toString<2,unsigned>(pressure)
		   << (--i ? "\'" : "");
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const FlatStroke& s) {
	for (std::size_t i = s.points.size()
	;    const Point& p : s.points) {
		os << Base36::toString<3,signed>(p.x)
		   << Base36::toString<3,signed>(p.y)
		   << (--i ? "\'" : "");
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const Marker& m) {
	return os << "(" << m.text << ")";
}

/* ~~ Print Modifiers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(std::ostream& os, const Mod::Affine& a) {
	const auto& m = a.matrix;
	os << "Affine [ "
	   << m[0] << " " << m[1] << " " << m[2] << " "
	   << m[3] << " " << m[4] << " " << m[5] << " "
	   << m[6] << " " << m[7] << " " << m[8] << " ";
	os << "]";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Mod::Array& a) {
	os << "Array [ "
	   << a.N << " "
	   << a.transformation << " ";
	os << "]";
	return os;
}

std::ostream& operator<<(std::ostream& os, const Mod::Uppercase& u) {
	return os << "Uppercase [ ]";
}