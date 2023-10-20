#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include "types.hh"

namespace ranges = std::ranges;

template <typename T>
class FileFormat { protected: using DataType = T;
public:
	virtual bool verify(std::istream& is/*, ParseError e*/) = 0;
	virtual DataType parse(std::istream& is) = 0;

	// Useful functions for parsing:
protected:
	static constexpr bool isWhitespace(char c) { return c==' ' || c=='\t' || c=='\n'; }
	static constexpr bool isLowercase (char c) { return 'a' <= c&&c <= 'z'; }
	static constexpr bool isUppercase (char c) { return 'A' <= c&&c <= 'Z'; }
	static constexpr bool isBase10    (char c) { return '0' <= c&&c <= '9'; }
	static constexpr bool isBase36    (char c) { return isBase10(c) || isLowercase(c) || isUppercase(c); }

	// This only needs to support at most 3 digits right now.
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



class RawFormat : FileFormat<RawSketch> {
public:
	bool verify(std::istream& is) final {
		enum { S0, X1, X2, Y1, Y2 } state = S0;
		for (char c; is.get(c); )
			if (isBase36(c))
				state = (state == S0) ? X1
				:       (state == X1) ? X2
				:       (state == X2) ? Y1
				:       (state == Y1) ? Y2
				:     /*(state == Y2)*/ X1;
			else if (isWhitespace(c) && (state == S0 || state == Y2))
				state = S0;
			else
				return false;
		return state == S0 || state == Y2;
	}

	DataType parse(std::istream& is) final {
		RawSketch result {};
		for (std::string line; is >> std::ws >> line; ) {
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
};