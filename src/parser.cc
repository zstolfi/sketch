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

auto SketchFormat::tokenize(std::string_view str)
-> std::expected<std::vector<Token>, ParseError> {
	std::vector<Token> result {};

	auto resultPush = [&](std::size_t i, std::size_t j) {
		result.emplace_back(str.substr(i, j-i));
	};

	enum {
		LineStart, Comment, Space, End,
		NumOrType, String, StringEnd, Operator,
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
			if (Util::isAny(c,":[],;")) return Operator;
			if (c == '(')         return String;
			else /* default:   */ return NumOrType;
		} (prevState, str[i]);

		if (prevState == Operator) {
			resultPush(i-1, i);
			if (str[i] == ';') return result;
		}

		if (prevState != nextState) {
			if (nextState == NumOrType) tokenStart = i;
			if (prevState == NumOrType) resultPush(tokenStart, i);

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

auto SketchFormat::sketchParse(TokenSpan tokens)
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

auto SketchFormat::elementParse(TokenSpan tokens)
-> std::expected<Element, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyElement);
	Parser* elemParser = tokens[0] == Token {"Data"  } ? typeDataParse
	:                    tokens[0] == Token {"Raw"   } ? typeRawParse
	:                    tokens[0] == Token {"Marker"} ? typeMarkerParse
	:                    /* default:                */   nullptr;

	if (!elemParser) return std::unexpected(UnknownElementType);

	return elemParser(
		Util::subspan(tokens, ++tokens.cbegin(), tokens.cend())
	);
}

/* ~~ Element Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::typeDataParse(TokenSpan tokens)
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

auto SketchFormat::typeRawParse(TokenSpan tokens)
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

auto SketchFormat::typeMarkerParse(TokenSpan tokens)
-> std::expected<Element, ParseError> {
	if (tokens.empty()) return std::unexpected(EmptyElement);

	auto string = atomStringParse({tokens.cbegin(), 1uz});
	if (!string) return std::unexpected(string.error());

	Element result {ElementType::Marker, {
		Marker {string}
	}};

	auto modifiers = modsMarkerParse(
		Util::subspan(tokens, ++tokens.cbegin(), tokens.cend())
	);
	if (!modifiers) return std::unexpected(modifiers.error());
	result.modifiers = std::move(*modifiers);

	return result;
}

/* ~~ Atom Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::atomStrokeDataParse(TokenSpan tokens)
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

auto SketchFormat::atomStrokeRawParse(TokenSpan tokens)
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

auto SketchFormat::atomStringParse(TokenSpan tokens)
-> std::expected<std::string, ParseError> {
	if (tokens.size() != 1) return std::unexpected(AtomSize);
	if (!isStringLiteral(tokens[0])) {
		return std::unexpected(MissingString);
	}

	return std::string {++tokens[0].begin(), --tokens[0].end()};
}

/* ~~ Modifier Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::modsStrokeParse(TokenSpan tokens)
-> std::expected<std::vector<Modifier>, ParseError> {
	std::vector<Modifier> result {};

	while (auto it = tokens.cbegin(); it != tokens.cend(); /**/) {
		Token type = *it++;
		Parser* modParser = (type == "Affine") ? modAffineParse
		:                   (type == "Array" ) ? modArrayParse
		:                   /*              */   nullptr;

		if (!elemParser) return std::unexpected(UnknownModifierType);

		auto contents = parenParse(tokens, it);
		if (!contents) return std::unexpected(contents.error());

		auto modifier = modParser(*contents);
		if (!modifier) return std::unexpected(modifer.error());

		std::advance(it, contents.size());
		result.modifiers.push_back(modifier);
	}

	return result;
}

auto SketchFormat::modsMarkerParse(TokenSpan tokens)
-> std::expected<std::vector<Modifier>, ParseError> {
	std::vector<Modifier> result {};

	while (auto it = tokens.cbegin(); it != tokens.cend(); /**/) {
		Token type = *it++;
		Parser* modParser = (type == "Uppercase") ? modAffineParse
		:                   /*                 */   nullptr;

		if (!elemParser) return std::unexpected(UnknownModifierType);

		auto contents = parenParse(tokens, it);
		if (!contents) return std::unexpected(contents.error());

		auto modifier = modParser(*contents);
		if (!modifier) return std::unexpected(modifer.error());

		std::advance(it, contents.size());
		result.modifiers.push_back(modifier);
	}

	return result;
}

auto SketchFormat::modAffineParse(TokenSpan tokens)
-> std::expected<Mod::Affine, ParseError> {
	if (tokens.size() != 1+9+1) return std::unexpected(ModAffineSize);
	if (tokens[ 0] != Token {"["}
	||  tokens[10] != Token {"]"}) {
		return std::unexpected(MalformedModAffine);
	}

	std::array<float,9> matrix {};
	for (std::size_t i=0; i<9; i++) {
		auto number = floatParse(tokens[1+i]);
		if (!number) return std::unexpected(number.error());
		matrix[i] = *number;
	}

	return Mod::Affine {matrix};
}

auto SketchFormat::modArrayParse(TokenSpan tokens)
-> std::expected<Mod::Array, ParseError> {
	if (tokens.size() != 4+9+2) return std::unexpected(ModArraySize);
	if (tokens[ 0] != Token {"["}
	||  tokens[ 2] != Token {"Affine"}
	||  tokens[ 3] != Token {"["}
	||  tokens[13] != Token {"]"}
	||  tokens[14] != Token {"]"}) {
		return std::unexpected(MalformedModArray);
	}

	auto n = integerParse(tokens[1]);
	if (!n) return std::unexpected(n.error());

	std::array<float,9> matrix {};
	for (std::size_t i=0; i<9; i++) {
		auto number = floatParse(tokens[4+i]);
		if (!number) return std::unexpected(number.error());
		matrix[i] = *number;
	}

	return Mod::Array {*n, matrix};
}

auto SketchFormat::modUppercaseParse(TokenSpan tokens)
-> std::expected<Mod::Uppercase, ParseError> {
	if (tokens.size() != 2) return std::unexpected(ModUppercaseSize);
	if (tokens[0] != Token {"["}
	||  tokens[1] != Token {"]"}) {
		return std::unexpected(MalformedModUppercase);
	}

	return Mod::Uppercase {};
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::isStringLiteral(Token tkn) -> bool {
	return tkn.string.size() >= 2
	&&     tkn.string.front() == '('
	&&     tkn.string.back()  == ')';
}

static auto parenParse(TokenSpan tokens, TokenIterator it)
-> std::expected<TokenSpan, ParseError> {
	/* ... */
	return Util::subspan(tokens, ++it, tokens.cend());
}

static auto integerParse(Token)
-> std::expected<int, ParseError> {
	/* ... */
	return int {1};
}

static auto floatParse(Token)
-> std::expected<float, ParseError> {
	/* ... */
	return float {1.0};
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