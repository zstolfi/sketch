#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <span>
#include <string>
#include <variant>
// #include <cstdint>
using namespace std::literals;

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

struct Pattern { /* ... */ };
struct Mask    { /* ... */ };
struct Eraser  { Mask shape; };
struct Marker  { std::string text; };
using  Atom    = std::variant<Stroke, Pattern, Eraser, Marker>;

/* ~~ Modifier Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

namespace Mod
{
	class Affine {
		std::array<float,9> m;
	public:
		Affine();
		Affine(std::array<float,9>);
		std::vector<Atom> operator()(std::span<const Atom> atoms);
	};

	class Array {
		Affine transformation;
		std::size_t n;
	public:
		Array(Affine tf, std::size_t n);
		std::vector<Atom> operator()(std::span<const Atom> atoms);
	};
};

using Modifier = std::variant<Mod::Affine, Mod::Array/*, ... */>;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum struct ElementType { Data, Pencil, Brush, Fill, Eraser, Letters };

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