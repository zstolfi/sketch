#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <span>
#include <string>
#include <variant>

/* ~~ .sketch Data Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct RawPoint  {
	int x, y;
};

struct RawStroke {
	std::vector<RawPoint> points;
};

struct RawSketch {
	using value_type = RawStroke;
	std::vector<RawStroke> strokes;
	void sendTo(std::ostream& os);
};

/* ~~ .HSC Data Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct Point {
	int x, y;
	float pressure;
	Point(int x, int y, float p);
	static Point fromRaw(const RawPoint& p);
};

struct Stroke {
	unsigned diameter;
	std::vector<Point> points;
	Stroke();
	Stroke(unsigned d, std::vector<Point> p);
	static Stroke fromRaw(const RawStroke& s);
};

// All default-constructible
struct Pattern { /* ... */ };
struct Mask    { /* ... */ };
struct Eraser  { Mask shape; };
struct Marker  { std::string text; };

using Atoms = std::variant<
	std::vector<Stroke>,
	/*std::vector<Pattern>,*/
	/*std::vector<Eraser>,*/
	std::vector<Marker>
>;

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Mod
{
	template <typename Atom>
	using Call_t = std::vector<Atom>(std::span<const Atom>) const;

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	class Affine {
		std::array<float,9> matrix;
	public:
		Affine();
		Affine(float);
		Affine(std::array<float,9>);
		auto operator*(Affine) const -> Affine;
		auto operator*(Point) const -> Point;
		Call_t<Stroke> operator();
	};

	class Array {
		std::size_t N;
		Affine transformation;
	public:
		Array(std::size_t n, Affine tf);
		Call_t<Stroke> operator();
	};

	using Of_Stroke = std::variant<Affine, Array>;

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
		Data,/* Pencil, Brush, Fill, Eraser, Letters,*/
		Marker,
	};

	Type type;
	Atoms atoms;
	Modifiers modifiers;
	Element(Type, Atoms);
	Element(Type, Atoms, Modifiers);
};

struct Sketch {
	using value_type = Element;
	std::vector<Element> elements;
	Sketch();
	Sketch(std::vector<Element>);
	static Sketch fromRaw(const RawSketch&);

	RawSketch flatten();
};



std::ostream& operator<<(std::ostream& os, const RawPoint&);
std::ostream& operator<<(std::ostream& os, const RawStroke&);
std::ostream& operator<<(std::ostream& os, const RawSketch&);

std::ostream& operator<<(std::ostream& os, const Point&);
std::ostream& operator<<(std::ostream& os, const Stroke&);
std::ostream& operator<<(std::ostream& os, const Marker&);
std::ostream& operator<<(std::ostream& os, const Sketch&);