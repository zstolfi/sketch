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
public:
	struct SourcePos {
		// (0,0) for an undefined row/col,
		// all valid values are 1-indexed.
		std::size_t row=0, col=0;

		void next();
		void next(char);
		auto operator<=>(const SourcePos&) const = default;
		friend std::ostream& operator<<(
			std::ostream&, const SourcePos&
		);
	};

	static constexpr auto SourcePos_unknown = SourcePos {0,0};


protected:
	struct Token {
		std::string_view string;
		SourcePos pos;

		constexpr bool operator==(const Token& other) const {
			return this->string == other.string;
		}
		constexpr auto operator<=>(const Token& other) const {
			return this->string <=> other.string;
		}
	};

	using TokenSpan = std::span<const Token>;
	using TokenIter = TokenSpan::/*const_*/iterator;

	struct ParseError {
		enum Type {
			EmptyFile, UnbalancedString, MissingSemicolon, MissingString,
			EmptyElement, UnknownElementType, UnknownModifierType,
			MissingBracketLeft, MissingBracketRight,
			MismatchingParens, AtomSize, StrokeLength, ForeignDigit,
			ModAffineSize, MalformedModAffine,
			ModArraySize, MalformedModArray,
			ModUppercaseSize, MalformedModUppercase,
			TickmarkOrdering, SignCharacter, NumberSize, NumberError,
		} type;

		SourcePos pos;

		constexpr ParseError(Type t) : type{t} {}
		constexpr ParseError(Type t, SourcePos pos) : type{t}, pos{pos} {}
		constexpr operator int() { return (int)type; }
	};

	static constexpr auto Unexpected(auto&& err) {
		return std::unexpected {ParseError {err}};
	}

	static constexpr auto Unexpected(auto&& err, SourcePos pos) {
		ParseError error {err};
		if (error.pos == SourcePos_unknown) error.pos = pos;
		return std::unexpected {error};
	}

	static constexpr auto Unexpected(auto&& err, Token tkn) {
		ParseError error {err};
		if (error.pos == SourcePos_unknown) error.pos = tkn.pos;
		return std::unexpected {error};
	}

	using enum ParseError::Type;

	template <typename T>
	using Expected = std::expected<T, ParseError>;

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