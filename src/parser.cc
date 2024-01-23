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

SketchFormat::Token::Token(std::string_view str) {
	this->string = str;
}

SketchFormat::Token::Token(std::string_view str,
	std::size_t i, std::size_t j
) {
	assert(i < j);
	this->string = str.substr(i, j-i);
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

auto SketchFormat::parse(std::string_view str)
-> std::expected<Sketch, ParseError> {
	auto tokens = tokenize(str);
	if (!tokens) return std::unexpected(tokens.error());
	return sketchParse(*tokens);
}

/* ~~ Top-level Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::sketchParse(std::span<const Token> tokens)
-> std::expected<Sketch, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyFile);
	if (!Util::contains(tokens, Token {";"})) {
		return std::unexpected(MissingSemicolon);
	}
	const auto delimEnd = ranges::find(tokens, Token {";"});

	Sketch result {};

	for (auto it = tokens.cbegin(); it != delimEnd; /**/) {
		if (*it == Token {","}) ++it;
		const auto delimNext = ranges::find(tokens, Token {","});
		const auto delim = Util::min(delimNext, delimEnd);

		auto element = elementParse(Util::subspan(tokens, it, delim));
		if (!element) return std::unexpected(element.error());
		result.elements.push_back(*element);
		it = delim;
	}

	return result;
}

auto SketchFormat::elementParse(std::span<const Token> tokens)
-> std::expected<Element, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyElement);
	Parser* elemParser = tokens[0] == Token {"Data"  } ? typeDataParse
	:                    tokens[0] == Token {"Raw"   } ? typeRawParse
	:                    tokens[0] == Token {"Marker"} ? typeMarkerParse
	:                    /*                         */   nullptr;

	if (elemParser == nullptr) return std::unexpected(UnknownElementType);

	return elemParser(
		Util::subspan(tokens, ++tokens.cbegin(), tokens.cend())
	);
}

/* ~~ Element Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::typeDataParse(std::span<const Token> tokens)
-> std::expected<Element, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyElement);
	if (tokens[0] != Token {"["}) {
		return std::unexpected(MissingBracketSqL);
	}
	if (!Util::contains(tokens, Token {"]"})) {
		return std::unexpected(MissingBracketSqR);
	}

	Element result {ElementType::Data, {}};

	auto it = ++tokens.cbegin();
	const auto delimElem = ranges::find(tokens, Token {"]"});
	result.atoms.reserve(std::distance(it, delimElem));

	for (; it != delimElem; ++it) {
		auto stroke = atomStrokeDataParse({it, 1uz});
		if (!stroke) return std::unexpected(stroke.error());
		result.atoms.push_back(*stroke);
	}

	auto modifiers = modsStrokeParse(
		Util::subspan(tokens, ++it, tokens.cend())
	);
	if (!modifiers) return std::unexpected(modifiers.error());
	result.modifiers = std::move(*modifiers);

	return result;
}

auto SketchFormat::typeRawParse(std::span<const Token> tokens)
-> std::expected<Element, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyElement);

	if (tokens[0] != Token {"["}) {
		return std::unexpected(MissingBracketSqL);
	}
	if (!Util::contains(tokens, Token {"]"})) {
		return std::unexpected(MissingBracketSqR);
	}

	Element result {ElementType::Data, {}};

	auto it = ++tokens.cbegin();
	const auto delimElem = ranges::find(tokens, Token {"]"});
	result.atoms.reserve(std::distance(it, delimElem));

	for (; it != delimElem; ++it) {
		auto stroke = atomStrokeRawParse({it, 1uz});
		if (!stroke) return std::unexpected(stroke.error());
		result.atoms.push_back(*stroke);
	}

	auto modifiers = modsStrokeParse(
		Util::subspan(tokens, ++it, tokens.cend())
	);
	if (!modifiers) return std::unexpected(modifiers.error());
	result.modifiers = std::move(*modifiers);

	return result;
}

auto SketchFormat::typeMarkerParse(std::span<const Token> tokens)
-> std::expected<Element, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyElement);
	if (tokens[0].string.front() != "("
	||  tokens[0].string.back()  != ")") {
		return std::unexpected(MissingString);
	}

	Element result {ElementType::Marker, {
		Marker ({++tokens[0].begin(), --tokens[0].end()})
	}};

	auto modifiers = modsMarkerParse(
		Util::subspan(tokens, ++tokens.cbegin(), tokens.cend())
	);
	if (!modifiers) return std::unexpected(modifiers.error());
	result.modifiers = std::move(*modifiers);

	return result;
}

/* ~~ Atom Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::atomStrokeDataParse(std::span<const Token> tokens)
-> std::expected<Stroke, ParseError> {
	if (tokens.size() != 1) return std::unexpected(AtomSize);

	auto strokeStr = removeTicks(tokens[0]);
	if (!strokeStr) return std::unexpected(strokeStr.error());
	auto& str = *strokeStr;

	Stroke result {};

	if (str.size()%6 != 0) return std::unexpected(StrokeLength);
	for (std::size_t i=0; i<str.size(); i+=6) {
		auto x = Base36::parse<3,signed>(str.substr(i+0, 3));
		auto y = Base36::parse<3,signed>(str.substr(i+3, 3));
		if (!x || !y) return std::unexpected(ForeignDigit);
		result.points.emplace_back(3, *x, *y);
	}
	return result;
}

auto SketchFormat::atomStrokeRawParse(std::span<const Token> tokens)
-> std::expected<Stroke, ParseError> {
	if (tokens.size() != 1) return std::unexpected(AtomSize);

	auto strokeStr = removeTicks(tokens[0]);
	if (!strokeStr) return std::unexpected(strokeStr.error());
	auto& str = *strokeStr;

	Stroke result {};

	if (str.size()%4 != 0) return std::unexpected(StrokeLength);
	for (std::size_t i=0; i<str.size(); i+=4) {
		auto x = Base36::parse<2,unsigned>(str.substr(i+0, 2));
		auto y = Base36::parse<2,unsigned>(str.substr(i+2, 2));
		if (!x || !y) return std::unexpected(ForeignDigit);
		result.points.emplace_back(3, *x, *y);
	}
	return result;
}

auto SketchFormat::atomStringParse(std::span<const Token> tokens)
-> std::expected<std::string, ParseError> {
	std::string result {};
	if (tokens.size() != 1) return std::unexpected(AtomSize);

	return std::string {tokens[0].string};
}

/* ~~ Modifier Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::modsStrokeParse(std::span<const Token> tokens)
-> std::expected<std::vector<Modifier>, ParseError> {
	std::vector<Modifier> result {};

	while (auto it = tokens.cbegin(); it != tokens.cend(); ++it) {
		Token type = *it++;
		Parser* modParser = (type == "Affine") ? modAffineParse
		:                   (type == "Array" ) ? modArrayParse
		:                   /*              */   nullptr;

		if (elemParser == nullptr) {
			return std::unexpected(UnknownModifierType);
		}

		auto modifier = modParser(Util::subspan(tokens, /* ... */));
		if (!modifier) return std::unexpected(modifer.error());

		result.modifiers.push_back(modifier);
	}

	return result;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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