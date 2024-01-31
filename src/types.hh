#pragma once
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <span>

/* ~~ .sketch Compatible Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct RawPoint  {
	int x, y;
};

struct RawStroke {
	std::vector<RawPoint> points;
};

struct RawSketch {
	std::vector<RawStroke> strokes;
};

/* ~~ .HSC Specific Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct Point {
	int x, y;
	double pressure;
	Point(int x, int y, double p);
};

struct Stroke {
	unsigned diameter;
	std::vector<Point> points;
	Stroke();
	Stroke(unsigned d, std::vector<Point>);
};

struct FlatPoint : Point {
	FlatPoint(int x, int y);
	static FlatPoint fromRaw(const RawPoint&);
};

struct FlatStroke : Stroke {
	FlatStroke();
	FlatStroke(std::vector<FlatPoint>);
	static FlatStroke fromRaw(const RawStroke&);
};

// struct Pattern { /* ... */ };
// struct Mask    { /* ... */ };
// struct Eraser  { Mask shape; };
struct Marker  { std::string text; };

using StrokeAtoms = std::vector<Stroke>;
using FlatStrokeAtoms = std::vector<FlatStroke>;
using Atoms = std::variant<
	StrokeAtoms,
	FlatStrokeAtoms,
	/*Pattern,*/
	/*std::vector<Eraser>,*/
	Marker
>;

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Mod
{
	template <typename Atom>
	using Call_t = std::vector<Atom>(std::span<const Atom>) const;

	/* ~~ Stroke Modifiers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	class Affine {
	public:
		std::array<double,9> matrix;
		Affine();
		Affine(double);
		Affine(std::array<double,9>);
		auto operator*(Affine) const -> Affine;
		auto operator*(Point) const -> Point;
		Call_t<Stroke> operator();
	};

	class Array {
	public:
		std::size_t N;
		Affine transformation;
		Array(std::size_t n, Affine tf);
		Call_t<Stroke> operator();
	};

	using Of_Stroke = std::variant<Affine, Array>;

	/* ~~ Marker Modifiers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	class Uppercase {
	public:
		Uppercase();
		Call_t<Marker> operator();
	};

	using Of_Marker = std::variant<Uppercase>;
}

using StrokeModifiers = std::vector<Mod::Of_Stroke>;
using MarkerModifiers = std::vector<Mod::Of_Marker>;
using Modifiers = std::variant<
	StrokeModifiers,
	MarkerModifiers
	/*, ... */
>;

/* ~~ Main Sketch Type ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct Element {
	enum Type {
		Brush, Pencil, Data,
		/* Fill, Eraser, Letters,*/
		Marker,
	};

	Type type;
	Atoms atoms;
	Modifiers modifiers;
	Element(Type, Atoms);
	Element(Type, Atoms, Modifiers);
};

struct FlatSketch;

struct Sketch {
	std::vector<Element> elements;
	Sketch();
	Sketch(std::vector<Element>);
	static Sketch fromRaw(const RawSketch&);
	auto render() -> RawSketch;
};

/* ~~ Print Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Raw Sketch
std::ostream& operator<<(std::ostream& os, const RawSketch&);

// Sketch
std::ostream& operator<<(std::ostream& os, const Sketch&);
std::ostream& operator<<(std::ostream& os, const Element&);
// Atoms
std::ostream& operator<<(std::ostream& os, const Stroke&);
std::ostream& operator<<(std::ostream& os, const FlatStroke&);
std::ostream& operator<<(std::ostream& os, const Marker&);
// Modifiers
std::ostream& operator<<(std::ostream& os, const Mod::Affine&);
std::ostream& operator<<(std::ostream& os, const Mod::Array&);
std::ostream& operator<<(std::ostream& os, const Mod::Uppercase&);