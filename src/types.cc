#include "types.hh"
#include "base36.hh"
#include "util.hh"
#include <algorithm>

/* ~~ .HSC Data Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Point::Point(int x, int y, float p)
: x{x}, y{y}, pressure{p} {}

Point Point::fromRaw(const RawPoint& p) {
	return Point {p.x, p.y, 1.0};
}

Stroke::Stroke()
: diameter{3} {}

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
	std::vector<Stroke> strokes {};
	strokes.reserve(s.strokes.size());

	for (const RawStroke& t : s.strokes) {
		strokes.push_back(Stroke::fromRaw(t));
	}

	self.elements = {Element {Element::Data, strokes}};
	return self;
}

Element::Element(Element::Type t, Atoms a)
: type{t}, atoms{a} {}

Element::Element(Element::Type t, Atoms a, Modifiers m)
: type{t}, atoms{a}, modifiers{m} {}

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Mod::Affine::Affine() : matrix{1,0,0 , 0,1,0 , 0,0,1} {}
Mod::Affine::Affine(float x) : matrix{x,0,0 , 0,x,0 , 0,0,x} {}
Mod::Affine::Affine(std::array<float,9> m) : matrix{m} {}

auto Mod::Affine::operator*(Affine other) const -> Affine {
	const auto& a = this->matrix, b = other.matrix;
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

auto Mod::Affine::operator*(Point p) const -> Point {
	return Point {
		int (matrix[0]*p.x + matrix[1]*p.y + matrix[2]),
		int (matrix[3]*p.x + matrix[4]*p.y + matrix[5]),
		/* .pressure */ 1.0,
	};
}

auto Mod::Affine::operator()(std::span<const Stroke> strokes) const
-> std::vector<Stroke> {
	std::vector<Stroke> result {};
	result.reserve(strokes.size());

	auto multiplyP = [this](Point p) -> Point {
		return (*this) * p;
	};

	auto multiplyS = [&](const Stroke& s) -> Stroke {
		return Stroke {
			s.diameter,
			s.points | views::transform(multiplyP)
			/*    */ | ranges::to<std::vector>(),
		};
	};

	ranges::copy(
		strokes | views::transform(multiplyS),
		std::back_inserter(result)
	);

	return result;
}

Mod::Array::Array(std::size_t n, Affine tf) : N{n}, transformation{tf} {}

auto Mod::Array::operator()(std::span<const Stroke> strokes) const
-> std::vector<Stroke> {
	std::vector<Stroke> result {};
	result.reserve(N * strokes.size());

	for (std::size_t i=0; i<N; i++) {
		ranges::copy(
			Util::pow(transformation, i)(strokes),
			std::back_inserter(result)
		);
	}

	return result;
}

Mod::Uppercase::Uppercase() {}

auto Mod::Uppercase::operator()(std::span<const Marker> markers) const
-> std::vector<Marker> {
	std::vector<Marker> result {};
	result.reserve(markers.size());

	auto toUpperC = [](char c) -> char {
		return ('a' <= c&&c <= 'z' ? c + ('A'-'a') : c);
	};

	auto toUpperM = [&](const Marker& m) -> Marker {
		return Marker {
			m.text | views::transform(toUpperC)
			/*  */ | ranges::to<std::string>()
		};
	};

	ranges::copy(
		markers | views::transform(toUpperM),
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
		os << e << (--i ? ",\n" : ";");
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const Element& element) {
	switch (element.type) {
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
		[&os](const std::vector<Marker>& markers) {
			// Doesn't really make sense to have
			// multiple markers in elements yet.
			os << markers[0];
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
	for (std::size_t i = s.points.size()
	;    const Point& p : s.points) {
		os << Base36::toString<3,signed>(p.x)
		   << Base36::toString<3,signed>(p.y);
		os << (--i ? "\'" : "");
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