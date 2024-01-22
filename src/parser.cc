#include "parser.hh"
#include "base36.hh"
#include "util.hh"
#include <string>

auto RawFormat::parse(std::string_view str)
-> std::expected<RawSketch, ParseError> {
	RawSketch result {};

	for (std::size_t i=0, j=0; i<str.size(); i=j) {
		while (i < str.size() && isWhitespace(str[i])) i++;
		if (i == str.size()) break;
		j = i;
		while (j < str.size() && !isWhitespace(str[j])) j++;

		RawStroke stroke {};
		if ((j-i)%4 != 0) return std::unexpected(StrokeLength);
		for (std::size_t k=i; k<j; k+=4) {
			auto x = Base36::parse<2,unsigned>(str.substr(k+0, 2));
			auto y = Base36::parse<2,unsigned>(str.substr(k+2, 2));
			if (!x || !y) return std::unexpected(ForeignDigit);
			stroke.points.emplace_back(*x, *y);
		}
		result.strokes.push_back(stroke);
	}

	return result;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

SketchFormat::Token::Token(std::string_view s,
	std::size_t i, std::size_t j
) {
	assert(i < j);
	this->string = s.substr(i, j-i);
}

auto SketchFormat::tokenize(std::string_view str)
-> std::expected<std::vector<Token>, ParseError> {
	std::vector<Token> result {};

	auto resultPush = [&](std::size_t i, std::size_t j) {
		result.emplace_back(str, i, j);
	};

	// Token includes types/numbers/base36.
	// Op is any single character operator.
	enum {
		LineStart, Comment, Space, End,
		Token, String, StringEnd, Op,
	}
	prevState = LineStart,
	nextState = LineStart;

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
			if (Util::isAny(c,":[],;")) return Op;
			if (c == '(')         return String;
			else /*  default:  */ return Token;
		} (prevState, str[i]);

		if (prevState == Op) {
			resultPush(i-1, i);
			if (str[i] == ';') return result;
		}

		if (prevState != nextState) {
			if (nextState == Token) tokenStart = i;
			if (prevState == Token) resultPush(tokenStart, i);

			if (nextState == String) tokenStart=i, parenCount=1;
			if (nextState == StringEnd) resultPush(tokenStart, i+1);

			if (prevState == String && nextState == End)
				return std::unexpected(UnbalancedString);
		}
	}

	return result;
}

auto SketchFormat::printTokens(std::string_view str) -> void {
	auto tokens = tokenize(str);
	if (tokens) {
		for (Token t : *tokens) {
			std::cout << "\t\"" << t.string << "\"\n";
		}
	}
	else {
		std::cout << "\t!! Invalid tokens input !!\n";
		std::cout << "\tError code: " << (int)tokens.error() << "\n";
	}
}