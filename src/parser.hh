#pragma once
#include "types.hh"
#include "math.hh"
#include "base36.hh"
#include "util.hh"
#include <iostream>
#include <map>
#include <string_view>
#include <variant>
#include <vector>
#include <concepts>
#include <expected>
#include <span>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

class ParserBase {
protected: // Useful functions for parsing:
	enum ParseError {
		EmptyFile, UnbalancedString, MissingSemicolon, MissingString,
		EmptyElement, UnknownElementType, UnknownModifierType,
		TickmarkOrdering,
		MissingBracketLeft, MissingBracketRight,
		MismatchingParens, AtomSize, StrokeLength, ForeignDigit,
		ModAffineSize, MalformedModAffine,
		ModArraySize, MalformedModArray,
		ModUppercaseSize, MalformedModUppercase,
		SignCharacter, NumberSize, NumberError
	};

	template <typename T>
	using Expected = std::expected<T, ParseError>;

	struct Token {
		std::string_view string;
		bool operator<=>(const Token&) const = default;
	};

	using TokenSpan = std::span<const Token>;
	using TokenIter = TokenSpan::/*const_*/iterator;

	static constexpr auto isWhitespace(char c) -> bool {
		return Util::isAny(c, " \t\n\r");
	}

	static constexpr auto isNewline(char c) -> bool {
		return Util::isAny(c, "\n\r");
	}

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	template <typename Base>
	static constexpr auto mapError(Base::ParseError err) -> ParseError {
		if (err == Base::ForeignDigit) return ForeignDigit;
		if (err == Base::StringSize  ) return NumberSize;
		return NumberError;
	};

	template <std::integral T>
	static auto integerParse(const Token tkn) -> Expected<T> {
		std::string_view str = tkn.string;

		int sign = 1;
		if (Util::isAny(str[0], "+-")) {
			if constexpr (std::is_signed_v<T>) {
				return std::unexpected(SignCharacter);
			}
			else /*std::is_unsigned_v<T>*/ {
				sign = (str[0] == '+') ? +1 : -1;
				str.remove_prefix(1);
			}
		}

		T result {};
		auto number = Base10::parse_n<T>(str);
		if (!number) {
			return std::unexpected(mapError<Base10>(number.error()));
		}
		result = *number;

		return sign * result;
	}

	template <std::floating_point T>
	static auto floatParse(const Token tkn) -> Expected<T> {
		std::string_view str = tkn.string;

		int sign = 1;
		if (Util::isAny(str[0], "+-")) {
			sign = (str[0] == '+') ? +1 : -1;
			str.remove_prefix(1);
		}

		T result {};
		if (std::size_t c = ranges::count(str, '.'); c == 0) {
			auto n = integerParse<std::size_t>(Token {str});
			if (!n) return std::unexpected(n.error());
			result = *n;
		}
		else if (c == 1) {
			auto headStr = str.substr(0uz, str.find('.'));
			auto tailStr = str.substr(str.find('.')+1);

			if (headStr.empty() && tailStr.empty()) {
				return std::unexpected(NumberSize);
			}

			T head=0, tail=0;
			if (!headStr.empty()) {
				auto h = integerParse<std::size_t>(Token {headStr});
				if (!h) return std::unexpected(h.error());
				head = *h;
			}
			if (!tailStr.empty()) {
				// Remove trailing 0's out of courtesy.
				tailStr = tailStr.substr(0uz,
					tailStr.find_last_not_of('0') + 1
				);
				
				auto t = integerParse<std::size_t>(Token {tailStr});
				if (!t) return std::unexpected(t.error());
				tail = *t;
			}

			result = head + tail * Util::pow(0.1, tailStr.size());
		}
		else return std::unexpected(ForeignDigit);

		return sign * result;
	}
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

class RawFormat : public ParserBase {
public:
	static auto parse(std::string_view) -> Expected<RawSketch>;

	// Maybe add a trivial RawFormat tokenizer
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

class SketchFormat : public ParserBase {
public:
	static auto parse(std::string_view) -> Expected<Sketch>;

private:
	static auto tokenize(std::string_view)
	-> Expected<std::vector<Token>>;

	static auto parenParse(TokenSpan, TokenIter)
	-> Expected<TokenSpan>;

	static auto isStringLiteral(const Token) -> bool;
	static auto removeTicks(const Token) -> Expected<std::string>;

	template <typename T>
	using Parser = auto (TokenSpan) -> Expected<T>;

	static Parser   <Sketch>                        sketchParse;
	static Parser /*└─*/<Element>                   elementParse;
	static Parser /*    │ */<Element>               typeDataParse;
	static Parser /*    │ */<Element>               typeRawParse;
	static Parser /*    │ */<Element>               typeMarkerParse;
	              /*    ├── <Atoms> */
	static Parser /*    │     */<Stroke>            atomStrokeDataParse;
	static Parser /*    │     */<Stroke>            atomStrokeRawParse;
	static Parser /*    │     */<Marker>            atomMarkerParse;
	static Parser /*    ├─*/<StrokeModifiers>       modsStrokeParse;
	static Parser /*    │     */<Mod::Of_Stroke>    modAffineParse;
	static Parser /*    │     */<Mod::Of_Stroke>    modArrayParse;
	static Parser /*    └─*/<MarkerModifiers>       modsMarkerParse;
	static Parser /*          */<Mod::Of_Marker>    modUppercaseParse;

public:
	static auto printTokens(std::string_view) -> void;
};