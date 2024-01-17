#pragma once
#include <vector>
#include <ranges>
#include <span>
#include <list>
#include <string>
#include <string_view>
#include <variant>
#include <map>
#include <cassert>
#include <cstdint>
namespace ranges = std::ranges;
using namespace std::literals;

struct Point;
struct Stroke;
struct Sketch;

struct RawPoint;
struct RawStroke;
struct RawSketch;

// Editor Data Types:
struct Point   { int16_t x, y; float pressure; };
struct Stroke  { unsigned diameter; std::vector<Point> points; };
struct Pattern { /* ... */ };
struct Mask    { /* ... */ };
struct Eraser  { Mask shape; };
struct Marker  { std::string text; };
using  Atom    = std::variant<Stroke, Pattern, Eraser, Marker>;

// Modifier Types:
class Affine {
	std::array<float,9> m;
public:
	Affine();
	Affine(std::array<float,9> m);
	std::vector<Atom> operator()(std::span<const Atom> atoms);
};

class Array {
	Affine transformation;
	std::size_t n;
public:
	std::vector<Atom> operator()(std::span<const Atom> atoms);
};

using Modifier = std::variant<Affine, Array/*, ... */>;

enum struct ElementType { Data, Pencil, Brush, Fill, Eraser, Letters };

// If the type is not found in the map, it
// means that type doesn't correspond to a
// "grouping" of elements. (i.e. Markers).
const std::map<std::string_view,ElementType> elementTypeFromString;

struct Element {
	struct AtomRange { std::list<Atom>::iterator begin, end; };
	ElementType type;
	AtomRange atoms;
	std::vector<Modifier> modifiers;
};

struct Sketch {
	std::list  <Atom>    atoms;
	std::vector<Element> elements;
	// RawSketch flatten();
};


// Raw Data Types:
struct RawPoint  {
	int16_t x, y;
};

struct RawStroke {
	std::vector<RawPoint> points;
};

struct RawSketch {
	std::vector<RawStroke> strokes;
};

std::ostream& operator<<(std::ostream& os, const Point& p);
std::ostream& operator<<(std::ostream& os, const Stroke& s);
std::ostream& operator<<(std::ostream& os, const Marker& m);
std::ostream& operator<<(std::ostream& os, const Sketch& s);