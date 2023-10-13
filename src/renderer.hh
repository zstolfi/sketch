#pragma once
#include <functional>
#include <algorithm>
#include "math.hh"

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

	void drawLine(RawPoint a, RawPoint b) {
		// auto [xMin, xMax] = std::minmax(a.x, b.x);
		// auto [yMin, yMax] = std::minmax(a.y, b.y);
		// unsigned x0=std::max<unsigned>(  0, xMin-2);
		// unsigned y0=std::max<unsigned>(  0, yMin-2);
		// unsigned x1=std::min<unsigned>(W-1, xMax+2);
		// unsigned y1=std::min<unsigned>(H-1, yMax+2);
		unsigned x0 = 0, x1 = W-1;
		unsigned y0 = 0, y1 = H-1;

		for (unsigned y=y0; y<=y1; y++)
		for (unsigned x=x0; x<=x1; x++) {
			float px = x - a.x + 0.5;
			float py = y - a.y + 0.5;
			float bx = b.x - a.x;
			float by = b.y - a.y;
			float c;
			// Circle case
			if (bx == 0 && by == 0)
				c = 255*clamp(std::hypot(px, py)-1, 0, 1);

			float len = std::hypot(bx, by);
			float h = (px*bx + py*by)/(bx*bx + by*by);
			c = 255*clamp(hypot(px - h*bx, py - h*by)-1, 0, 1);
			// set (darken)
			auto& pixel = pixels[y*W+x];
			if ((pixel >> 8 & 0xFF) > c) // Temporary hack
				pixel = MapRGB(c,c,c);
		}
	}

	void displayRaw(const RawSketch& sketch) {
		for (RawStroke s : sketch) {
			if (s.size() == 1) { drawLine(s[0], s[0]); continue; }
			for (std::size_t i=1; i<s.size(); i++) {
				drawLine(s[i-1], s[i]);
			}
		}
	}

};