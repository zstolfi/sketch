#include "renderer.hh"
#include "graphics.hh"
#include "util.hh"
#include <algorithm>

Renderer::Renderer(
	std::span<uint32_t> output,
	unsigned W, unsigned H,
	std::function<uint32_t(Col3)> map,
	std::function<Col3(uint32_t)> get
)
: pixels{output}, W{W}, H{H}
, MapRGB{map}, GetRGB{get} {}

/* ~~ Drawing Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void Renderer::displayRaw(std::span<const Atom::FlatStroke> strokes) {
	for (const Atom::FlatStroke& s : strokes) {
		if (s.points.size() < 2) continue;

		auto toVec2 = [](Atom::FlatStroke::Point p) {
			return Vec2 {Real(p.x), Real(p.y)};
		};

		// ranges::for_each(
		// 	s.points
		// 	| views::transform(toVec2)
		// 	| views::slide(2)
		// 	, [&](auto&& r) { drawLine(r[0], r[1]); }
		// );
		for (std::size_t i=0; i<s.points.size()-1; i++) {
			drawLine(
				toVec2(s.points[i+0]),
				toVec2(s.points[i+1])
			);
		}
	}
}

void Renderer::clear() {
	for (std::size_t i=0; i<W*H; i++) {
		pixels[i] = MapRGB({255,255,255});
	}
}

// TODO: more efficient line draw function
void Renderer::drawLine(Vec2 a, Vec2 b) {
	auto [xMin, xMax] = std::minmax(a.x, b.x);
	auto [yMin, yMax] = std::minmax(a.y, b.y);
	const Real x0 = max(  0, xMin-2);
	const Real y0 = max(  0, yMin-2);
	const Real x1 = min(W-1, xMax+2);
	const Real y1 = min(H-1, yMax+2);

	const Vec2 av = {a.x, a.y};
	const Vec2 bv = {b.x, b.y};

	Vec2 xy;
	for (Real y=y0; y<=y1; y+=1) {
	for (Real x=x0; x<=x1; x+=1) {
		xy = Vec2 {x + 0.5, y + 0.5};
		uint8_t c = 255*clamp(
			SDFline(xy, av, bv) - 1,
			0, 1
		);
		auto& pixel = pixels[y*W + x];
		Col3 cOld = GetRGB(pixel);
		// TODO: basic blending modes
		pixel = MapRGB({
			(cOld.r < c) ? cOld.r : c,
			(cOld.g < c) ? cOld.g : c,
			(cOld.b < c) ? cOld.b : c,
		});
	} }
}