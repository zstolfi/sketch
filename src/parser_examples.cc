/*
	em++ parser_examples.cc -std=c++23 -sUSE_SDL=2 -o parser.html
*/

#include <emscripten.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <expected>
#include <cstdint>

#include "window.hh"

struct RawPoint  { int16_t x, y; };
using  RawStroke = std::vector<RawPoint>;
using  RawSketch = std::vector<RawStroke>;
auto operator<<(std::ostream&, const RawPoint &) -> std::ostream&;
auto operator<<(std::ostream&, const RawStroke&) -> std::ostream&;
auto operator<<(std::ostream&, const RawSketch&) -> std::ostream&;



auto isWhitespace(char c) -> bool {
	return c == ' ' || c == '\t'; }

// Only working with lowercase for now.
auto isBase36Digit(char c) -> bool {
	return ('0' <= c&&c <= '9')
	||     ('a' <= c&&c <= 'z'); }

// Likewise here
auto parseBase36Digit(char c) -> int {
	return ('0' <= c&&c <= '9') ? c-'0' : c-'a' + 10; }

auto operator>>(std::istream& is, RawStroke& stroke) -> std::istream&
{
	std::string line;
	std::getline(is >> std::ws, line, ' ');
	if (line.size()%4 == 0
	&&  line.size() > 0
	&&  std::all_of(line.begin(), line.end(), isBase36Digit)) {
		stroke = {};
		for (std::size_t i=0; i < line.size(); i+=4)
			stroke.emplace_back(
				36*parseBase36Digit(line[i+0]) // X
				+  parseBase36Digit(line[i+1]),
				36*parseBase36Digit(line[i+2]) // Y
				+  parseBase36Digit(line[i+3]) 
			);
		return is;
	}
	is.setstate(std::ios_base::failbit);
	return is;
}

auto operator>>(std::istream& is, RawSketch& sketch) -> std::istream&
{
	sketch = {};
	for (RawStroke s; is >> s; )
		sketch.push_back(s);
	return is;
}




auto main() -> int
{
	Window window {"Parser Examples", 800, 600};

	std::istringstream example {" 0000m8go  m80000go  \t0000111122223333"};

	RawSketch sketch;
	example >> sketch;
	std::cout << sketch << "\n";

	/* TODO: Sketch renderer */
	unsigned char r=200, g=200, b=200;
	for (std::size_t i=0; i < window.width() * window.height(); i++) {
		window.pixels[i] = SDL_MapRGB(window.format, r, g, b);
	}

	window.updatePixels();
	emscripten_set_main_loop([]{}, 0, true);
}



auto operator<<(std::ostream& os, const RawPoint& p) -> std::ostream&
{
	return os << "(" << p.x << "," << p.y << ")";
}

auto operator<<(std::ostream& os, const RawStroke& s) -> std::ostream&
{
	auto it = s.begin();
	os << *it++;
	while (it != s.end())
		os << "---" << *it++;
	return os;
}

auto operator<<(std::ostream& os, const RawSketch& s) -> std::ostream&
{
	auto it = s.begin();
	if (it == s.end()) return os << "[]";
	os << "[ " << *it++;
	while (it != s.end())
		os << ", " << *it++;
	return os << " ]";
}