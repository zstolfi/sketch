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

	template <std::integral T>
	static auto integerParse(const Token tkn) -> Expected<T> {
		/* ... */
		return T {1};
	}

	template <std::floating_point T>
	static auto floatParse(const Token tkn) -> Expected<T> {
		/* ... */
		return T {1.0};
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