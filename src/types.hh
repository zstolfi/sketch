#pragma once
#include <vector>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <map>
#include <cassert>
#include <cstdint>
namespace ranges = std::ranges;
namespace views  = std::views;
using namespace    std::literals;

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

namespace Mod
{
	class Affine;
	class Array;
	#include "modifiers.hh"
};
using namespace Mod;

using Modifier = std::variant<Affine, Array/*, ... */>;

enum struct ElementType { Data, Pencil, Brush, Fill, Eraser, Lettering };

// If the type is not found in the map, it
// means that type doesn't correspond to a
// "grouping" of elements. (i.e. Markers).
const std::map<std::string_view,ElementType> elementTypeFromString {
	{"Data"  , ElementType::Data  },
	{"Pencil", ElementType::Pencil},
	{"Brush" , ElementType::Brush },
	{"Fill"  , ElementType::Fill  },
};

struct Element {
	ElementType           type;
	std::span<const Atom> atoms;
	std::vector<Modifier> modifiers;
};

struct Sketch {
	std::vector<Atom>    atoms;
	std::vector<Element> elements;
};


// Raw Data Types:
struct RawPoint  {
	int16_t x, y;

	operator Point() const {
		return Point {.x=x, .y=y, .pressure=1.0};
	}
};

struct RawStroke {
	std::vector<RawPoint> points;

	operator Stroke() const {
		Stroke s {};
		s.points.reserve(points.size());
		for (RawPoint p : points)
			s.points.push_back(static_cast<Point>(p));
		s.diameter = 3;
		return s;
	}
};

struct RawSketch {
	std::vector<RawStroke> strokes;

	operator Sketch() const {
		Sketch s {};
		s.atoms.reserve(strokes.size());
		for (RawStroke t : strokes)
			s.atoms.emplace_back(static_cast<Stroke>(t));
		// s.elements = {
		// 	Element {
		// 		ElementType::Data,
		// 		std::span { s.atoms },
		// 		.modifiers = {}
		// 	};
		// };
		return s;
	}
};

#include "types print.hh"