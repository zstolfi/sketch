#pragma once
#include "types.hh"
#include "math.hh"
#include <functional>
#include <span>

struct Col3 { uint8_t r, g, b; };

class Renderer {
	// Possibly an mdspan in the future.
	std::span<uint32_t> pixels;
	const unsigned W = 800;
	const unsigned H = 600;
	std::function<uint32_t(Col3)> MapRGB;
	std::function<Col3(uint32_t)> GetRGB;

	void drawLine(RawPoint a, RawPoint b);

public:
	Renderer(
		std::span<uint32_t>,
		unsigned W, unsigned H,
		std::function<uint32_t(Col3)> map,
		std::function<Col3(uint32_t)> get
	);

	void clear();
	void displayRaw(std::span<const RawSketch::value_type>);
	void display(std::span<const Sketch::value_type>);
};