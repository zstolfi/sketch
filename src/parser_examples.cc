/*
	em++ parser_examples.cc -std=c++23 -sUSE_SDL=2 -o parser.html
*/

#include <emscripten.h>
#include <iostream>
#include <string_view>
#include <vector>
#include <algorithm>
#include <expected>
#include <cstdint>

#include "window.hh"

struct Point { int16_t x, y; };
using Line = std::pair<Point,Point>;
using Lines = std::vector<Line>;
auto operator<<(std::ostream&, const Point&) -> std::ostream&;
auto operator<<(std::ostream&, const Line &) -> std::ostream&;
auto operator<<(std::ostream&, const Lines&) -> std::ostream&;


auto isWhitespace(char c) -> bool {
	return c == ' ' || c == '\t'; }

// Only working with lowercase for now.
auto isBase36Digit(char c) -> bool {
	return ('0' <= c&&c <= '9')
	||     ('a' <= c&&c <= 'z'); }

// Likewise here
auto parseBase36Digit(char c) -> int {
	return ('0' <= c&&c <= '9') ? c-'0' : c-'a' + 10; }

enum struct praseError {
	Early_EOF,
	Early_Whitespace,
	Invalid_Character,
	Out_Of_Bounds_Point,
};

// /* ITERATION 1: DFA approach ... I gave up */
// auto parseRaw (std::string_view str)
// 	-> std::expected<Lines, praseError>
// {
// 	using enum praseError;
// 	Lines result {};
// 	int16_t    x=0,    y=0;
// 	int16_t xOld=0, yOld=0;

// 	enum { S0, X1, X2, Y1, Y2 } state = S0;
// 	for (auto it=str.begin(); it!=str.end(); ++it) {
// 		if (isBase36Digit(*it)) {
// 			switch (state) {
// 			case S0: case X1:
// 				x = 36*x + parseBase36Digit(*it);
// 				break;
// 			case X2: case Y1:
// 				y = 36*y + parseBase36Digit(*it);
// 				break;
// 			case Y2:
// 				result.emplace_back(
// 					{xOld,yOld},
// 					{x,y}
// 				);
// 				xOld = x, yOld = y;
// 				break;
// 			}

// 			state = (state == S0) ? X1
// 			:       (state == X1) ? X2
// 			:       (state == X2) ? Y1
// 			:       (state == Y1) ? Y2
// 			:      /*state == Y2*/  X1;
// 		}
// 		else if (isWhitespace(*it)) {
// 			if (state == Y2) {
// 				result.emplace_back(x,y);
// 				xOld = x, yOld = y;
// 				x=0, y=0;
// 			}

// 			if (state == S0 || state == Y2)
// 				state = S0;
// 			else
// 				return std::unexpected(Early_Whitespace);
// 		}
// 		else
// 			return std::unexpected(Invalid_Character);
// 	}
// 	if (state != S0 && state != Y2)
// 		return std::unexpected(Early_EOF);
// 	else
// 		return result;
// }

// /* ITERATION 2: I realize I need a parseRawPoint func */
// auto parseRaw(std::string_view str)
// 	-> std::expected<Lines, praseError>
// {
// 	using enum praseError;
// 	Lines result {};
// 	int16_t x=0, y=0;

// 	auto decodePos = [](auto begin, auto end)
// 	{
// 		int16_t result = 0;
// 		while (begin != end)
// 			result = 36*result + parseBase36Digit(*begin++);
// 		return result;
// 	};

// 	auto it=str.begin(), end=str.end();
// 	while (it != end) {
// 		if (isWhitespace(*it) || it==str.begin()) {
// 			while (isWhitespace(*it) && it!=end)
// 				++it;

// 			if (std::distance(it,end) < 4)
// 				return std::unexpected(Early_EOF);
// 			if (!std::all_of(it, std::next(it,4), isBase36Digit))
// 				return std::unexpected(Invalid_Character);

// 			x = decodePos(it, it+=2);
// 			y = decodePos(it, it+=2);
// 		}
// 		else if (isBase36Digit(*it)) {

// 			if (std::distance(it,end) < 4)
// 				return std::unexpected(Early_EOF);
// 			if (!std::all_of(it, std::next(it,4), isBase36Digit))
// 				return std::unexpected(Invalid_Character);

// 			result.emplace_back(
// 				Point {
// 					x,
// 					y
// 				},
// 				Point {
// 					x = decodePos(it, it+=2),
// 					y = decodePos(it, it+=2)
// 				}
// 			);
// 		}

// 		else
// 			return std::unexpected(Invalid_Character);
// 	}
// 	return result;
// }

/* ITERATION 3: */
// TODO: rewrite yet one more time with istreams
//       (maybe even with the operator>> syntax)
auto parseRawPoint(std::string_view str)
	-> std::expected<Point, praseError>
{
	using enum praseError;
	Point result {};

	if (str.size() < 4)
		return std::unexpected(Early_EOF);
	// TODO: return error when size > 4, maybe?
	if (std::any_of(str.begin(), str.end(), isWhitespace))
		return std::unexpected(Early_Whitespace);
	if (!std::all_of(str.begin(), str.end(), isBase36Digit))
		return std::unexpected(Invalid_Character);

	result.x = 36*parseBase36Digit(str[0]) + parseBase36Digit(str[1]);
	result.y = 36*parseBase36Digit(str[2]) + parseBase36Digit(str[3]);

	if (!(0 <= result.x && result.x <= 800)
	||  !(0 <= result.y && result.y <= 600))
		return std::unexpected(Out_Of_Bounds_Point);
	else
		return result;
}

auto parseRaw(std::string_view str)
	-> std::expected<Lines, praseError>
{
	using enum praseError;
	Lines result {};
	Point p;

	std::size_t i=0, size=str.size();
	while (i < str.size()) {
		if (isWhitespace(str[i]) || i==0) {
			while (isWhitespace(str[i]) && i<size)
				i++;
			if (i>=size) break;
			if (auto pNew = parseRawPoint(str.substr(i, 4))) {
				p = *pNew;
				i += 4;
			}
			else
				return std::unexpected(pNew.error());
		}
		else if (isBase36Digit(str[i])) {
			if (auto pNew = parseRawPoint(str.substr(i, 4))) {
				result.emplace_back(p, *pNew);
				p = *pNew;
				i += 4;
			}
			else
				return std::unexpected(pNew.error());
		}
		else
			return std::unexpected(Invalid_Character);
	}
	return result;
}



auto main() -> int
{
	Window window {"Parser Examples", 800, 600};

	std::string_view example = "0000m8go   m80000go  \t0000111122223333";

	if (auto lines = parseRaw(example))
		std::cout << *lines << "\n";
	else
		std::cout << "ERROR! code: " << (int)lines.error() << "\n";

	/* TODO: Sketch renderer */
	unsigned char r=200, g=200, b=200;
	for (std::size_t i=0; i < window.width() * window.height(); i++) {
		window.pixels[i] = SDL_MapRGB(window.format, r, g, b);
	}

	window.updatePixels();
	emscripten_set_main_loop([]{}, 0, true);
}



auto operator<<(std::ostream& os, const Point& p) -> std::ostream&
{
	return os << "(" << p.x << "," << p.y << ")";
}

auto operator<<(std::ostream& os, const Line& l) -> std::ostream&
{
	return os << l.first << " -> " << l.second;
}

auto operator<<(std::ostream& os, const Lines& l) -> std::ostream&
{
	auto it = l.begin();
	if (it == l.end()) return os << "[]";
	os << "[ " << *it++;
	while (it != l.end())
		os << ", " << *it++;
	return os << " ]";
}