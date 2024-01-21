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

auto SketchFormat::tokenize(std::string_view str) -> std::vector<Token> {
	std::vector<Token> result {};

	// auto resultAdd = [&](std::size_t i0, std::size_t i1) {
	// 	result.push_back(str.substr(i0, i1-i0));
	// };

	// // Token includes types/numbers/base36.
	// // Op is any single character operator.
	// enum {
	// 	LineStart, Comment, Space, End,
	// 	Token, String, StringEnd, Op
	// }
	// prevState = LineStart,
	// nextState;

	// std::size_t tokenStart=0;
	// int parenCount = 0;
	// for (std::size_t i=0; i<=str.size(); i++, prevState = nextState) {
	// 	nextState = i==str.size() ? End :
	// 	[&](auto state, char c) {
	// 		if (state == LineStart && c == '%') return Comment;
	// 		if (state == Comment)
	// 			return isNewline(c) ? LineStart : Comment;
	// 		if (state == String) {
	// 			if (c == '(') ++parenCount;
	// 			if (c == ')' && --parenCount <= 0) return StringEnd;
	// 			return String;
	// 		}

	// 		if (isNewline(c))     return LineStart;
	// 		if (isWhitespace(c))  return Space;
	// 		if (Util::isAny(c,":[],;")) return Op;
	// 		if (c == '(')         return String;
	// 		else /*            */ return Token;
	// 	} (prevState, str[i]);

	// 	if (prevState == Op) {
	// 		resultAdd(i-1, i);
	// 		if (str[i] == ';') return result;
	// 	}

	// 	if (prevState != nextState) {
	// 		if (nextState == Token) tokenStart = i;
	// 		if (prevState == Token) resultAdd(tokenStart, i);

	// 		if (nextState == String) tokenStart=i, parenCount=1;
	// 		if (nextState == StringEnd) resultAdd(tokenStart, i+1);

	// 		// The source file ends with an unterminated string
	// 		// literal. Which is probably because of unbalanced
	// 		// parentheses in the string. The tokenizer doesn't
	// 		// error because the validity can be checked later.
	// 		if (prevState == String && nextState == End)
	// 			resultAdd(tokenStart, i);
	// 	}
	// }

	return result;
}