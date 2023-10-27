#pragma once
#include <functional>
#include <algorithm>
#include "types.hh"
#include "math.hh"

struct Col3 { uint8_t r, g, b; };

class Renderer {
	// Possibly an mdspan in the future.
	std::span<uint32_t> pixels;
	const unsigned W = 800;
	const unsigned H = 600;
	std::function<uint32_t(Col3)> MapRGB;
	std::function<Col3(uint32_t)> GetRGB;

public:
	Renderer(std::span<uint32_t> output,
	         unsigned W, unsigned H,
	         std::function<uint32_t(Col3)> map,
	         std::function<Col3(uint32_t)> get)
	: pixels{output}, W{W}, H{H}
	, MapRGB{map}, GetRGB{get} {}

	void clear() {
		for (std::size_t i=0; i<W*H; i++)
			pixels[i] = MapRGB({255,255,255});
	}

	// TODO: more efficient line draw function
	void drawLine(RawPoint a, RawPoint b) {
		auto [xMin, xMax] = std::minmax(a.x, b.x);
		auto [yMin, yMax] = std::minmax(a.y, b.y);
		Real x0 = max(  0, xMin-2);
		Real y0 = max(  0, yMin-2);
		Real x1 = min(W-1, xMax+2);
		Real y1 = min(H-1, yMax+2);

		Vec2 av = {(Real)a.x, (Real)a.y};
		Vec2 bv = {(Real)b.x, (Real)b.y};
		Vec2 xy;

		for (Real y=y0; y<=y1; y+=1)
		for (Real x=x0; x<=x1; x+=1) {
			xy.x = x + 0.5;
			xy.y = y + 0.5;
			uint8_t c = 255*clamp(
				SDFline(xy, av, bv) - 1,
				0,
				1
			);
			auto& pixel = pixels[y*W+x];
			Col3 cOld = GetRGB(pixel);
			pixel = MapRGB({
				(cOld.r < c) ? cOld.r : c,
				(cOld.g < c) ? cOld.g : c,
				(cOld.b < c) ? cOld.b : c
			});
		}
	}

	void displayRaw(const RawSketch& sketch) {
		for (RawStroke s : sketch.strokes) {
			auto& p = s.points;
			if (p.size() == 1) { drawLine(p[0], p[0]); continue; }
			for (std::size_t i=1; i<p.size(); i++) {
				drawLine(p[i-1], p[i]);
			}
		}
	}

};