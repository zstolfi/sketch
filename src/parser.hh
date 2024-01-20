#pragma once
#include "types.hh"
#include "math.hh"
#include "util.hh"
#include <iostream>
#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>
#include <concepts>
using namespace std::literals;

class ParserBase {
protected: // Useful functions for parsing:
	static constexpr bool isWhitespace(char c) { return Util::isAny(c,' ','\t','\n','\r'); }
	static constexpr bool isNewline   (char c) { return Util::isAny(c,'\n','\r'); }
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
	static bool verify(std::istream& is);
	static RawSketch parse(std::istream& is);
};

class SketchFormat : public ParserBase {
	using Token  = std::string_view;
	using Tokens = std::vector<Token>;

public:
	// Removes whitespace/comments.
	static Tokens tokenize(std::string_view str);

private:
	enum ValueType { tBase36, tNumber, tString };

	struct tNone      { };
	struct tSingle    { ValueType type; };
	struct tBounded   { ValueType type; std::size_t n; };
	struct tUnbounded { ValueType type; std::size_t mult=1;};

	using V = std::variant<tNone, tSingle, tUnbounded, tBounded>;

	static constexpr auto ElementsDefs = std::array {
		// Compiler weirdly complains when the ", 1" is removed.
		std::pair { "Data"sv  , V{ tUnbounded{tBase36, 1} }},
		std::pair { "Pencil"sv, V{ tUnbounded{tBase36, 1} }},
		std::pair { "Brush"sv , V{ tUnbounded{tBase36, 2} }},
		std::pair { "Affine"sv, V{ tBounded  {tNumber, 9} }},
		std::pair { "Marker"sv, V{ tSingle   {tString   } }},
		/* TODO: Mask */
	};

	// If the type is not found in the map, it
	// means that type doesn't correspond to a
	// "grouping" of elements. (i.e. Markers).
	inline static const std::map elementTypeFromString {
		std::pair {"Data"sv  , ElementType::Data  },
		std::pair {"Pencil"sv, ElementType::Pencil},
		std::pair {"Brush"sv , ElementType::Brush },
		std::pair {"Fill"sv  , ElementType::Fill  },
	};

	struct ElementData {
		Token type;
		std::span<const Token> members;
	};

	// TODO: change std::optional to std::expected
	static auto parseElement(const Tokens& tkn, std::size_t& i)
	-> std::optional<ElementData>;

public:
	static auto parse(const Tokens& tkn)
	-> std::optional<Sketch>;
};