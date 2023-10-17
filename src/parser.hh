#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include "types.hh"

namespace ranges = std::ranges;

class Parser {
public:
	auto raw(std::istream& is) -> RawSketch {
		RawSketch result {};
		for (std::string line; is >> std::ws >> line; ) {
			if (line.size() % 4 != 0
			||  !ranges::all_of(line, isBase36)) {
				is.setstate(is.failbit);
				break;
			}
			RawStroke stroke {};
			for (std::size_t i=0; i<line.size(); i+=4) {
				RawPoint point {};
				point.x = base36(line.substr(i+0, 2));
				point.y = base36(line.substr(i+2, 2));
				stroke.push_back(point);
			}
			result.push_back(stroke);
		}
		return result;
	}

private:
	static constexpr bool isWhitespace(char c) { return c==' ' || c=='\t' || c=='\n'; }
	static constexpr bool isLowercase (char c) { return 'a' <= c&&c <= 'z'; }
	static constexpr bool isUppercase (char c) { return 'A' <= c&&c <= 'Z'; }
	static constexpr bool isBase10    (char c) { return '0' <= c&&c <= '9'; }
	static constexpr bool isBase36    (char c) { return isBase10(c) || isLowercase(c) || isUppercase(c); }

	// The max this needs to support right now is str.size() == 3
	static constexpr int16_t base36(std::string_view str) {
		int16_t result = 0;
		for (char c : str) {
			assert(isBase36(c));
			result = 36*result + (
				    isBase10   (c) ? c-'0'
				:   isLowercase(c) ? c-'a'+10
				: /*isUppercase(c)*/ c-'A'+10
			);
		}
		return result;
	}
};