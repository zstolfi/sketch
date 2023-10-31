#pragma once
#include <vector>
#include <variant>
#include <cstdint>

struct RawPoint;
struct RawStroke;
struct RawSketch;

// Editor Data Types:
struct Point   { int16_t x, y; float pressure; };
struct Stroke  { unsigned diameter; std::vector<Point> points; };
struct Pattern { /* ... */ };
struct Mask    { /* ... */ };
struct Eraser  { Mask m; };
using  Atom    = std::variant<Stroke, Pattern, Eraser/*, Marker*/>;
struct Group   { /* ... */ };
struct Sketch  {
	std::vector<Atom>  atoms;
	std::vector<Group> groups;
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
		// s.groups = {
		// 	Group {
		// 		GroupKind::Data,
		// 		std::span { s.atoms }
		// 	};
		// };
		return s;
	}
};
