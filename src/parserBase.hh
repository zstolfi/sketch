#pragma once
#include "base36.hh"
#include "util.hh"
#include <iostream>
#include <string_view>
#include <span>
#include <concepts>
#include <expected>

class ParserBase {
protected:
	enum ErrorType {
		EmptyFile, UnbalancedString, MissingSemicolon, MissingString,
		EmptyElement, UnknownElementType, UnknownModifierType,
		ElementBrushSize,
		MissingBracketLeft, MissingBracketRight,
		MismatchingParens, AtomSize, StrokeLength, ForeignDigit,
		ModAffineSize, MalformedModAffine,
		ModArraySize, MalformedModArray,
		ModUppercaseSize, MalformedModUppercase,
		TickmarkOrdering, SignCharacter, NumberSize, NumberError,
	};

public:
	struct SourcePos {
		// (0,0) for an undefined row/col,
		// all valid values are 1-indexed.
		std::size_t row=0, col=0;

		void next() { col++; }
		void next(char c) { c == '\n' ? row++, col=0 : col++; }
		auto operator<=>(const SourcePos&) const = default;
		friend std::ostream& operator<<(std::ostream&, const SourcePos&);
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
	using TokenIter = TokenSpan::iterator;

	struct ParseError {
		using Type = ErrorType;

		Type type;
		SourcePos pos;
		constexpr ParseError(Type t) : type{t} {}
		constexpr ParseError(Type t, SourcePos pos) : type{t}, pos{pos} {}
		constexpr operator int() { return int(type); }
	};

	using enum ParseError::Type;

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	template <typename T>
	using Expected = std::expected<T, ParseError>;

	static constexpr auto Unexpected(auto&& e) {
		return std::unexpected {ParseError {e}};
	}

	static constexpr auto Unexpected(auto&& e, SourcePos pos) {
		ParseError error {e};
		if (error.pos == SourcePos_unknown) error.pos = pos;
		return std::unexpected {error};
	}

	static constexpr auto Unexpected(auto&& e, Token tkn) {
		ParseError error {e};
		if (error.pos == SourcePos_unknown) error.pos = tkn.pos;
		return std::unexpected {error};
	}

	template <typename Base>
	static constexpr auto errorFrom(Base::ParseError e) -> ParseError {
		if (e == Base::ForeignDigit) return ForeignDigit;
		if (e == Base::StringSize)   return NumberSize;
		return NumberError;
	};

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	static constexpr auto isWhitespace(char c) -> bool {
		return Util::isAny(c, " \t\n\r");
	}

	static constexpr auto isNewline(char c) -> bool {
		return Util::isAny(c, "\n\r");
	}

	template <std::integral T>
	static auto integerParse(const Token tkn) -> Expected<T>;

	template <std::floating_point T>
	static auto floatParse(const Token tkn) -> Expected<T>;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

template <std::integral T>
auto ParserBase::integerParse(const Token tkn) -> Expected<T> {
	auto str = tkn.string;

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
		return std::unexpected(errorFrom<Base10>(number.error()));
	}
	result = *number;

	return sign * result;
}

template <std::floating_point T>
auto ParserBase::floatParse(const Token tkn) -> Expected<T> {
	auto str = tkn.string;

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
		auto tailStr = str.substr(str.find('.') + 1);

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