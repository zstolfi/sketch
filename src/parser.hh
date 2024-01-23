#pragma once
#include "types.hh"
#include "math.hh"
#include "util.hh"
#include <iostream>
#include <map>
#include <string_view>
#include <variant>
#include <vector>
#include <concepts>
#include <expected>
#include <span>

class ParserBase {
protected: // Useful functions for parsing:
	static constexpr bool isWhitespace(char c) { return Util::isAny(c," \t\n\r"); }
	static constexpr bool isNewline   (char c) { return Util::isAny(c,"\n\r"); }
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

	// TODO: base10 'verify' functions. These two functions
	//       accept all valid inputs but also invalid ones.

	// Parse integer constants
	template <std::integral T=int>
	static constexpr T base10(std::string_view str) {
		int sign = 1;
		switch (str.front()) {
			case '+': str.remove_prefix(1); /*      */ break;
			case '-': str.remove_prefix(1); sign = -1; break;
		}

		T result = 0;
		for (char c : str) {
			assert(isBase10(c));
			result = 10*result + (c-'0');
		}
		if constexpr (std::is_signed_v<T>)
			if (sign == -1) result = -result;
		return result;
	}

	// Parse floating point
	template <std::floating_point T=double>
	static constexpr T base10(std::string_view str) {
		using U = int;
		if (std::size_t dot = str.find('.'); dot != str.npos) {
			int sign = 1;
			switch (str.front()) {
				case '+': str.remove_prefix(1), dot--; /*      */ break;
				case '-': str.remove_prefix(1), dot--; sign = -1; break;
			}

			T result = 0;
			const std::size_t intSize  = dot;
			const std::size_t fracSize = str.size()-(dot+1);

			if (intSize)
				result += base10<U>(str.substr(0,dot));
			if (fracSize)
				result += base10<U>(str.substr(dot+1, fracSize))
				       *  pow((T)0.1, fracSize);

			return sign*result;
		}
		return (T)base10<U>(str);
	}
};

class RawFormat : public ParserBase {
public:
	enum ParseError {
		StrokeLength,
		ForeignDigit,
	};

	static auto parse(std::string_view)
	-> std::expected<RawSketch, ParseError>;
};

class SketchFormat : public ParserBase {
public:
	enum ParseError {
		EmptyFile,
		UnbalancedString,
		MissingSemicolon,
		MissingString,
		EmptyElement,
		UnknownElementType,
		UnknownModifierType,
		TickmarkOrdering,
		MissingBracketSqL, // [
		MissingBracketSqR, // ]
		AtomSize,
		StrokeLength,
		ForeignDigit,
	};

	static auto parse(std::string_view)
	-> std::expected<Sketch, ParseError>;

private:
	struct Token {
		std::string_view string;
		constexpr Token(std::string_view);
		Token(std::string_view, std::size_t i, std::size_t j);
		bool operator<=>(const Token&) const = default;
	};

	static auto tokenize(std::string_view)
	-> std::expected<std::vector<Token>, ParseError>;

	template <typename T>
	using Parser = auto (std::span<const Token>)
	/*          */ -> std::expected<T, ParseError>;

	static Parser   <Sketch>                        sketchParse;
	static Parser /*└─*/<Element>                   elementParse;
	static Parser /*    │ */<std::vector<Stroke>>   typeDataParse;
	static Parser /*    │ */<std::vector<Stroke>>   typeRawParse;
	static Parser /*    │ */<std::string>           typeMarkerParse;
	              /*    ├── <Atom> */
	static Parser /*    │     */<Stroke>            atomStrokeDataParse;
	static Parser /*    │     */<Stroke>            atomStrokeRawParse;
	static Parser /*    │     */<std::string>       atomStringParse;
	              /*    ├── <Stroke Modifier> */
	static Parser /*    │ */<std::vector<Modifier>> modsStrokeParse;
	static Parser /*    │     */<Mod::Affine>       modAffineParse;
	static Parser /*    │     */<Mod::Array>        modArrayParse;
	static Parser /*    └── <Marker Modifier> */
	static Parser /*      */<std::vector<Modifier>> modsmarkerParse;
	static Parser /*          */<Mod::Uppercase>    modUppercaseParse;

public:
	static auto printTokens(std::string_view) -> void;
};