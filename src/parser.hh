#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include "types.hh"

namespace ranges = std::ranges;
using namespace std::literals;

namespace /*anonymous*/ {
	// Useful functions for parsing:
	constexpr bool isWhitespace(char c) { return c==' ' || c=='\t' || c=='\n' || c=='\r'; }
	constexpr bool isNewline   (char c) { return c=='\n' || c=='\r'; }
	constexpr bool isLowercase (char c) { return 'a' <= c&&c <= 'z'; }
	constexpr bool isUppercase (char c) { return 'A' <= c&&c <= 'Z'; }
	constexpr bool isBase10    (char c) { return '0' <= c&&c <= '9'; }
	constexpr bool isBase36    (char c) { return isBase10(c) || isLowercase(c) || isUppercase(c); }

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

namespace RawFormat {
	bool verify(std::istream& is) {
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

	RawSketch parse(std::istream& is) {
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

namespace SketchFormat {
	using Tokens = std::vector<std::string_view>;
	bool isOperator(char c) { return ":[],;"sv.contains(c); }

	// Removes whitespace/comments.
	Tokens tokenize(std::string_view str) {
		Tokens result;
		auto resultAdd = [&](std::size_t i0, std::size_t i1) {
			result.push_back(str.substr(i0, i1-i0));
		};

		// Token includes types/numbers/base36.
		// Op is any single character operator.
		enum {
			LineStart, Comment, Space, Token,
			String, StringEnd, Op,
			End
		}
		prevState = LineStart,
		nextState;

		std::size_t tokenStart=0;
		int parenCount = 0;
		for (std::size_t i=0; i<=str.size(); i++, prevState = nextState) {
			nextState = i==str.size() ? End :
			[&](auto state, char c) {
				if (state == LineStart && c == '%') return Comment;
				if (state == Comment)
					return isNewline(c) ? LineStart : Comment;
				if (state == String) {
					if (c == '(') ++parenCount;
					if (c == ')' && --parenCount <= 0) return StringEnd;
					return String;
				}

				if (isNewline(c))     return LineStart;
				if (isWhitespace(c))  return Space;
				if (isOperator(c))    return Op;
				if (c == '(')         return String;
				/*                 */ return Token;
			} (prevState, str[i]);

			if (prevState == Op) resultAdd(i-1, i);
			if (prevState != nextState) {
				if (nextState == Token) tokenStart = i;
				if (prevState == Token) resultAdd(tokenStart, i);

				if (nextState == String) tokenStart=i, parenCount=1;
				if (nextState == StringEnd) resultAdd(tokenStart, i+1);

				// The source file ends with an unterminated string
				// literal. Which is probably because of unbalanced
				// parentheses in the string. The tokenizer doesn't
				// error because the validity can be checked later.
				if (prevState == String && nextState == End)
					resultAdd(tokenStart, i);
			}
		}

		return result;
	}
};