#pragma once
#include "types.hh"
#include "graphics.hh"
#include <functional>
#include <span>


class Renderer {
	std::span<uint32_t> pixels;
	const unsigned W = 800;
	const unsigned H = 600;
	std::function<uint32_t(Col3)> MapRGB;
	std::function<Col3(uint32_t)> GetRGB;

	void drawLine(Vec2 a, Vec2 b);

public:
	Renderer(
		std::span<uint32_t> output,
		unsigned W, unsigned H,
		std::function<uint32_t(Col3)> map,
		std::function<Col3(uint32_t)> get
	);

	void clear();
	void displayRaw(std::span<const Atom::FlatStroke>);
	// void display(std::span<const Elements>);
};