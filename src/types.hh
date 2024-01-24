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
using  Atom    = std::variant<Stroke/*, Pattern, Eraser*/, Marker>;

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Mod
{
	using Function_t = std::vector<Atom>(std::span<const Atom>) const;

	class Affine {
		using Atom_t = Stroke;
		std::array<float,9> m;
	public:
		Affine();
		Affine(float);
		Affine(std::array<float,9>);
		constexpr auto operator*(Affine) const -> Affine;
		constexpr auto operator*(Point) const -> Point;
		Function_t operator();
	};

	class Array {
		using Atom_t = Stroke;
		std::size_t n;
		Affine transformation;
	public:
		Array(std::size_t n, Affine tf);
		Function_t operator();
	};

	class Uppercase {
		using Atom_t = Marker;
	public:
		Uppercase();
		Function_t operator();
	};
}

using Modifier = std::variant<
	Mod::Affine, Mod::Array,
	Mod::Uppercase
	/*, ... */
>;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum struct ElementType {
	Data,/* Pencil, Brush, Fill, Eraser, Letters,*/
	Marker,
};

struct Element {
	ElementType type;
	std::vector<Atom> atoms;
	std::vector<Modifier> modifiers;
	Element(ElementType, std::vector<Atom>);
	Element(ElementType, std::vector<Atom>, std::vector<Modifier>);
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