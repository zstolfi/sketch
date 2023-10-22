#pragma once
#include <vector>
#include <variant>
#include <cstdint>

struct RawPoint;
struct RawStroke;
struct RawSketch;

// Editor Data Types:
struct Point   { int16_t x, y; float pressure; };
struct Stroke  : std::vector<Point> { unsigned diameter; };
struct Pattern { /* ... */ };
struct Mask    { /* ... */ };
struct Eraser  : Stroke {};
struct Element : std::variant<Stroke, Pattern, Eraser> {};
struct Group   { /* ... */ };
struct Sketch  {
	std::vector<Element> elements;
	std::vector<Group> groups;
};


// Raw Data Types:
struct RawPoint  {
	int16_t x, y;
	operator Point() const {
		return Point {.x=x, .y=y, .pressure=1.0};
	}
};

struct RawStroke : std::vector<RawPoint> {
	operator Stroke() {
		Stroke s {};
		s.reserve(this->size());
		for (RawPoint p : *this)
			s.push_back(static_cast<Point>(p));
		s.diameter = 3;
		return s;
	}
};

struct RawSketch : std::vector<RawStroke> {
	operator Sketch() const {
		Sketch s {};
		s.elements.reserve(this->size());
		for (RawStroke t : *this)
			s.elements.emplace_back(static_cast<Stroke>(t));
		// s.groups = {
		// 	Group {
		// 		GroupKind::Data,
		// 		s.elements.begin(),
		// 		s.elements.end()
		// 	};
		// };
		return s;
	}
};
