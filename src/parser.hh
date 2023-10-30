#pragma once
#include <iostream>
#include <algorithm>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <concepts>
#include <cassert>
#include "types.hh"
#include "math.hh"
namespace ranges = std::ranges;
namespace views  = std::views;
using namespace    std::literals;

class ParserBase {
protected: // Useful functions for parsing:
	static constexpr bool isWhitespace(char c) { return isAny(c,' ','\t','\n','\r'); }
	static constexpr bool isNewline   (char c) { return isAny(c,'\n','\r'); }
	static constexpr bool isLowercase (char c) { return 'a' <= c&&c <= 'z'; }
	static constexpr bool isUppercase (char c) { return 'A' <= c&&c <= 'Z'; }
	static constexpr bool isBase10    (char c) { return '0' <= c&&c <= '9'; }
	static constexpr bool isBase36    (char c) { return isBase10(c) || isLowercase(c) || isUppercase(c); }

	template <std::size_t N, std::integral T=signed>
	static constexpr T base36(std::string_view str) {
		// Check 36^N <= 2^(Bits in T)
		static_assert(N*Log2_36 <= 8*sizeof(T));
		assert(str.size() <= N);

		constexpr unsigned exp = pow(36u, N);
		T result = 0;
		for (char c : str) {
			assert(isBase36(c));
			result = 36*result + (
				    isBase10   (c) ? c-'0'
				:   isLowercase(c) ? c-'a'+10
				: /*isUppercase(c)*/ c-'A'+10
			);
		}
		if constexpr (std::is_signed_v<T>)
			if (result >= exp/2) result -= exp;
		return result;
	}

	// Parse integer constants
	template <std::integral T=int>
	static constexpr T base10(std::string_view str) {
		return 1; // PH
	}

	// Parse floating point
	template <std::floating_point T=double>
	static constexpr T base10(std::string_view str) {
		return 1.0; // PH
	}

	template <typename... Ts>
	struct Overloaded : Ts... { using Ts::operator()...; };

	template <typename T, typename... Ts>
	static constexpr bool isAny(T x, Ts... args) {
		auto compare = [](T x, auto arg) {
			// Specialization for looking up chars in a string:
			if constexpr (std::is_convertible_v<decltype(arg), std::string_view>)
				return std::string_view{arg}.contains(x);
			else
				return x == arg;
		};

		return (compare(x,args) || ... );
	}
};

class RawFormat : public ParserBase {
public:
	static bool verify(std::istream& is) {
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

	static RawSketch parse(std::istream& is) {
		RawSketch result {};
		for (std::string line; is >> std::ws >> line; ) {
			RawStroke stroke {};
			for (std::size_t i=0; i<line.size(); i+=4) {
				RawPoint point {};
				point.x = base36<3,int16_t>(line.substr(i+0, 2));
				point.y = base36<3,int16_t>(line.substr(i+2, 2));
				stroke.points.push_back(point);
			}
			result.strokes.push_back(stroke);
		}
		return result;
	}
};

class SketchFormat : public ParserBase {
	using Token  = std::string_view;
	using Tokens = std::vector<Token>;

public:
	// Removes whitespace/comments.
	static Tokens tokenize(std::string_view str) {
		Tokens result;
		auto resultAdd = [&](std::size_t i0, std::size_t i1) {
			result.push_back(str.substr(i0, i1-i0));
		};

		// Token includes types/numbers/base36.
		// Op is any single character operator.
		enum {
			LineStart, Comment, Space, End,
			Token, String, StringEnd, Op
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
				if (isAny(c,":[],;")) return Op;
				if (c == '(')         return String;
				/*                 */ return Token;
			} (prevState, str[i]);

			if (prevState == Op) {
				resultAdd(i-1, i);
				if (str[i] == ';') return result;
			}

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

private:
	enum ValueType { tBase36, tNumber, tString };

	struct tNone      { };
	struct tSingle    { ValueType type; };
	struct tBounded   { ValueType type; std::size_t n; };
	struct tUnbounded { ValueType type; std::size_t mult=1;};

	using V = std::variant<tNone, tSingle, tUnbounded, tBounded>;

	static constexpr auto Elements = std::array {
		// Compiler weirdly complains when the ", 1" is removed.
		std::pair { "Data"sv  , V{ tUnbounded{tBase36, 1} }},
		std::pair { "Pencil"sv, V{ tUnbounded{tBase36, 1} }},
		std::pair { "Brush"sv , V{ tUnbounded{tBase36, 2} }},
		std::pair { "Affine"sv, V{ tBounded  {tNumber, 9} }},
		std::pair { "Marker"sv, V{ tSingle   {tString   } }},
		/* TODO: Mask */
	};

	struct ElementData {
		Token type;
		std::span<const Token> members;
	};

	static auto parseElement(const Tokens& tkn, std::size_t& i)
	-> std::optional<ElementData> {
		if (i+1 > tkn.size()) return {};
		Token typeName = tkn[i++];

		auto match = ranges::find_if(Elements,
			[&](auto t) { return t.first == typeName; }
		);
		if (match == Elements.end()) return {};

		using Ret_t = std::optional<ElementData>;
		return std::visit(Overloaded {
			[&](tNone v) -> Ret_t {
				return ElementData {typeName, {}};
			},
			[&](tSingle v) -> Ret_t {
				if (i+2 > tkn.size()) return {};
				if (tkn[i++] != ":") return {};
				return ElementData {typeName, {&tkn[i++], 1}};
			},
			[&](tBounded v) -> Ret_t {
				if (i+2 > tkn.size()) return {};
				if (tkn[i++] != ":") return {};
				if (tkn[i++] != "[") return {};
				std::size_t count = 0;
				while (i < tkn.size() && tkn[i] != "]")
					i++, count++;
				if (i++ == tkn.size()) return {};
				if (count != v.n) return {};
				return ElementData {typeName, {&tkn[i-1-count], count}};
			},
			[&](tUnbounded v) -> Ret_t {
				if (i+2 > tkn.size()) return {};
				if (tkn[i++] != ":") return {};
				if (tkn[i++] != "[") return {};
				std::size_t count = 0;
				while (i < tkn.size() && tkn[i] != "]")
					i++, count++;
				if (i++ == tkn.size()) return {};
				return ElementData {typeName, {&tkn[i-1-count], count}};
			}
		}, match->second);
	}

public:
	static auto parse(const Tokens& tkn)
	-> std::optional<Sketch> {
		if (tkn.empty()) return {};
		if (tkn[0] == ";") return Sketch {};

		Sketch result {};
		for (std::size_t i=0; i<tkn.size(); i++) {
			std::vector<ElementData> elemsList {};
			while (i<tkn.size() && !isAny(tkn[i], ",", ";")) {
				if (auto e = parseElement(tkn, i))
					elemsList.push_back(*e);
				else
					return {};
			}

			// All 'statements' must contain > 0 elements.
			if (elemsList.empty()) return {};
			auto currElem = elemsList.begin();

			/* PARSE MAIN ELEMENT */
			std::vector<Element> timelineElems {};
			if (isAny(currElem->type, "Data", "Pencil", "Brush")) {
				const bool isBrush = currElem->type == "Brush";
				for (std::size_t j=0; j<currElem->members.size(); /**/) {
					unsigned diameter = isBrush
						? base36<2,unsigned>(currElem->members[j++])
						: 3;
					Stroke stroke {diameter, {}};
					std::string digits {};
					for (char c : currElem->members[j++]) {
						if (c == '\'') continue;
						digits.push_back(c);
						if (digits.size() < (isBrush? 8:6)) continue;
						stroke.points.push_back(Point {
							.x = base36<3,int16_t>(digits.substr(0, 3)),
							.y = base36<3,int16_t>(digits.substr(3, 3)),
							.pressure = isBrush
								? base36<2,unsigned>(digits.substr(6,2))
									/ float(36*36-1)
								: 1.0f
						});
						digits.clear();
					}
					assert(digits.empty());
					timelineElems.push_back(stroke);
				}
			}
			++currElem;

			/* PARSE ALL MODIFIERS */
			for (; currElem != elemsList.end(); ++currElem) {
				if (currElem->type == "Affine") {
					std::array<float,9> m;
					for (std::size_t j=0; j<9; j++)
						m[j] = base10<float>(currElem->members[j]);
					// std::array<float,9> m {
					// 	1, 0, 100,
					// 	0, 1, 100,
					// 	0, 0, 1,
					// };

					for (Element e : timelineElems) {
						assert(std::holds_alternative<Stroke>(e));
						Stroke& stroke = std::get<Stroke>(e);
						// TODO: Stroke scaling for non Pencil elements
						// stroke.diameter *= scaleFactor;
						for (Point& p : stroke.points) {
							std::cout << p.x << ", " << p.y << "\n";
							p = Point {
								.x = int16_t(p.x*m[0] + p.y*m[1] + m[2]),
								.y = int16_t(p.x*m[3] + p.y*m[4] + m[5]),
								.pressure = p.pressure
							};
							std::cout << "\t" << p.x << ", " << p.y << "\n";
						}
					}
				}
			}

			for (Element& e : timelineElems)
				result.elements.push_back(std::move(e));

			if (i<tkn.size() && tkn[i] == ";") break;
		}
		return result;
	}
};