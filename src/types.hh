#pragma once
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <span>
#include "util.hh"

/* ~~ Atom Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Atom
{
	struct FlatStroke {
		struct Point {
			int x, y;
			Point(int x, int y);
		};
		std::vector<Point> points;

		FlatStroke();
		FlatStroke(std::vector<Point>);
	};

	struct Stroke {
		struct Point {
			int x, y; double pressure;
			Point(int x, int y, double);
		};
		unsigned diameter;
		std::vector<Point> points;

		Stroke();
		Stroke(unsigned d, std::vector<Point>);
		Stroke(const FlatStroke&);
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

#define Concept(Name, ...) \
	template <typename T> concept Name = Util::IsAnyType<T, __VA_ARGS__>
Concept(HoldsStrokeAtoms    , Brush);
Concept(HoldsFlatStrokeAtoms, Pencil, Data);
Concept(HoldsMarkerAtom     , Marker);

Concept(HoldsStrokeMods     , Brush, Pencil, Data);
Concept(HoldsMarkerMods     , Marker);
#undef Concept

using Element = std::variant<
	Brush, Pencil, Data,
	Marker
>;

/* ~~ Main Sketch Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct FlatSketch {
	std::vector<Atom::FlatStroke> strokes;

	FlatSketch();
	FlatSketch(std::vector<Atom::FlatStroke>);
};

struct Sketch {
	std::vector<Element> elements;

	Sketch();
	Sketch(std::vector<Element>);
	Sketch(const FlatSketch&);
	auto render() const -> FlatSketch;
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