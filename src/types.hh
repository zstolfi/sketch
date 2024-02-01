#pragma once
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <span>

/* ~~ Atom Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Atom
{
	struct Stroke {
		struct Point {
			int x, y; double pressure;
			Point(int x, int y, double);
		};
		unsigned diameter;
		std::vector<Point> points;

		Stroke();
		Stroke(unsigned d, std::vector<Point>);
	};

	struct FlatStroke {
		struct Point {
			int x, y;
			Point(int x, int y);
		};
		std::vector<Point> points;

		FlatStroke();
		FlatStroke(std::vector<Point>);
		operator Stroke();
	};

	// struct Pattern { /* ... */ };
	// struct Mask    { /* ... */ };
	// struct Eraser  { Mask shape; };
	struct Marker  { std::string text; };
}

using StrokeAtoms     = std::vector<Atom::Stroke>;
using FlatStrokeAtoms = std::vector<Atom::FlatStroke>;
using MarkerAtom      = Atom::Marker;

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Mod
{
	template <typename A>
	using Call_t = std::vector<A>(std::span<const A>) const;

	/* ~~ Stroke Modifiers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	class Affine {
	public:
		std::array<double,9> matrix;
		Affine();
		Affine(double);
		Affine(std::array<double,9>);
		auto operator*(Affine) const -> Affine;
		Call_t<Atom::Stroke> operator();
	};

	class Array {
	public:
		std::size_t N;
		Affine transformation;
		Array(std::size_t n, Affine tf);
		Call_t<Atom::Stroke> operator();
	};

	using Of_Stroke = std::variant<Affine, Array>;

	/* ~~ Marker Modifiers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	class Uppercase {
	public:
		Uppercase();
		Call_t<Atom::Marker> operator();
	};

	using Of_Marker = std::variant<Uppercase>;
}

using StrokeModifiers = std::vector<Mod::Of_Stroke>;
using MarkerModifiers = std::vector<Mod::Of_Marker>;

/* ~~ Elements Type ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

template <typename A, typename M>
struct Element_Base { A atoms; M modifiers; };

struct Brush  : Element_Base <StrokeAtoms    , StrokeModifiers> {};
struct Pencil : Element_Base <FlatStrokeAtoms, StrokeModifiers> {};
struct Data   : Element_Base <FlatStrokeAtoms, StrokeModifiers> {};
struct Marker : Element_Base <MarkerAtom     , MarkerModifiers> {};

using Element = std::variant<
	Brush, Pencil, Data,
	Marker
>;

/* ~~ Main Sketch Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct Sketch;
struct FlatSketch {
	std::vector<Atom::FlatStroke> strokes;

	FlatSketch();
	FlatSketch(std::vector<Atom::FlatStroke>);
	operator Sketch();
};

struct Sketch {
	std::vector<Element> elements;

	Sketch();
	Sketch(std::vector<Element>);
	auto render() -> FlatSketch;
};

/* ~~ Print Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Sketch
std::ostream& operator<<(std::ostream& os, const Sketch&);
std::ostream& operator<<(std::ostream& os, const Element&);
// Atoms
std::ostream& operator<<(std::ostream& os, const StrokeAtoms&);
std::ostream& operator<<(std::ostream& os, const FlatStrokeAtoms&);
std::ostream& operator<<(std::ostream& os, const MarkerAtom&);
// Modifiers
std::ostream& operator<<(std::ostream& os, const StrokeModifiers);
std::ostream& operator<<(std::ostream& os, const MarkerModifiers);