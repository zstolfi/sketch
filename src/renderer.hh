#pragma once
#include <functional>
#include <cmath>

// Raw Data Types:
struct RawPoint  { int16_t x, y; };
struct RawStroke : std::vector<RawPoint>  {};
struct RawSketch : std::vector<RawStroke> {};

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

class Renderer {
	// Possibly an mdspan in the future.
	std::span<uint32_t> pixels;
	const unsigned W = 800;
	const unsigned H = 600;
	using RGB_Fn = uint32_t(uint8_t,uint8_t,uint8_t);
	std::function<RGB_Fn> MapRGB;

public:
	Renderer(std::span<uint32_t> output,
	         unsigned W, unsigned H,
	         std::function<RGB_Fn> map)
	: pixels{output}, W{W}, H{H}, MapRGB{map} {}

	void clear() {
		for (std::size_t i=0; i<W*H; i++)
			pixels[i] = MapRGB(255,255,255);
	}

	void displayRaw(const RawSketch& sketch) {
		/* ... */
	}

};