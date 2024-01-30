#include "parsers.hh"
#include "util.hh"
#include <string>
#include <stack>
#include <optional>

void ParserBase::SourcePos::next() {
	col++;
}

void ParserBase::SourcePos::next(char c) {
	if (c == '\n') row++, col=0; else col++;
}

/* ~~ Raw Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto RawFormat::parse(std::string_view str)
-> Expected<RawSketch> {
	return rawParse(tokenize(str));
}

auto RawFormat::tokenize(std::string_view str)
-> std::vector<Token> {
	std::vector<Token> result {};

	for (auto strokeStr : views::split(str, " ")) {
		result.emplace_back(std::string_view {strokeStr});
	}

	return result;
}

auto RawFormat::rawParse(TokenSpan tokens)
-> Expected<RawSketch> {
	RawSketch result {};

	for (Token tkn : tokens) {
		RawStroke stroke {};
		auto str = tkn.string;

		if (str.size()%4 != 0) return Unexpected(StrokeLength);
		for (std::size_t i=0; i<str.size(); i+=4) {
			auto x = Base36::parse<2,unsigned>(str.substr(i+0, 2));
			auto y = Base36::parse<2,unsigned>(str.substr(i+2, 2));
			if (!x || !y) return Unexpected(ForeignDigit);
			stroke.points.emplace_back(*x, *y);
		}
		result.strokes.push_back(stroke);
	}

	return result;
}

/* ~~ Sketch Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::parse(std::string_view str)
-> Expected<Sketch> {
	auto tokens = tokenize(str);
	if (!tokens) return Unexpected(tokens.error());
	return sketchParse(*tokens);
}

auto SketchFormat::tokenize(std::string_view str)
-> Expected<std::vector<Token>> {
	std::vector<Token> result {};
	SourcePos pos {1,1};

	auto resultPush = [&](std::size_t i, std::size_t j) {
		// TODO: Improve token pos accuracy.
		result.emplace_back(str.substr(i, j-i), pos);
	};

	enum State {
		LineStart, Comment, Space, End,
		NumOrType, String, StringEnd, Operator,
	}
	prevState = LineStart,
	nextState = LineStart;

	std::size_t tokenStart=0;
	int parenCount = 0;
	for (std::size_t i=0
	;    i<=str.size()
	;    pos.next(str[i++]), prevState = nextState) {
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

			if (isNewline(c))          return LineStart;
			if (isWhitespace(c))       return Space;
			if (Util::isAny(c,"[],;")) return Operator;
			if (c == '(')              return String;
			else /* default:        */ return NumOrType;
		} (prevState, str[i]);


		if (prevState == Operator) {
			resultPush(i-1, i);
			if (str[i-1] == ';') return result;
		}

		if (prevState != nextState) {
			if (nextState == NumOrType) tokenStart = i;
			if (prevState == NumOrType) resultPush(tokenStart, i);

			if (nextState == String) tokenStart=i, parenCount=1;
			if (nextState == StringEnd) resultPush(tokenStart, i+1);

			if (prevState == String && nextState == End)
				return Unexpected(UnbalancedString, pos);
		}
	}

	return result;
}

/* ~~ Top-level Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::sketchParse(TokenSpan tokens)
-> Expected<Sketch> {
	if (tokens.empty()) return Unexpected(EmptyFile);
	if (!Util::contains(tokens, Token {";"})) {
		return Unexpected(MissingSemicolon, tokens.back());
	}
	const auto delimEnd = ranges::find(tokens, Token {";"});

	Sketch result {};

	for (auto it = tokens.begin(); it != delimEnd; /**/) {
		if (*it == Token {","} && it != tokens.begin()) ++it;
		const auto delimNext = std::find(it, tokens.end(), Token {","});
		const auto delim = Util::min(delimNext, delimEnd);

		auto element = elementParse(Util::subspan(tokens, it, delim));
		if (!element) return Unexpected(element.error(), *it);
		result.elements.push_back(*element);
		it = delim;
	}

	return result;
}

auto SketchFormat::elementParse(TokenSpan tokens)
-> Expected<Element> {
	if (tokens.empty()) return Unexpected(EmptyElement);
	Parser<Element>* elemParser =
	  tokens[0] == Token {"Data"  } ? typeDataParse
	: tokens[0] == Token {"Raw"   } ? typeRawParse
	: tokens[0] == Token {"Marker"} ? typeMarkerParse
	: /* default:                */   nullptr;
	if (!elemParser) return Unexpected(UnknownElementType, tokens[0]);

	return elemParser(
		Util::subspan(tokens, ++tokens.begin(), tokens.end())
	);
}

/* ~~ Element Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::typeDataParse(TokenSpan tokens)
-> Expected<Element> {
	if (tokens.empty()) return Unexpected(EmptyElement);
	if (tokens[0] != Token {"["}) return Unexpected(MissingBracketLeft);

	auto it = tokens.begin();
	auto contents = parenParse(tokens, it);
	if (!contents) return Unexpected(contents.error(), *it);

	std::vector<Stroke> strokes {};
	strokes.reserve(contents->size());

	for (const Token tkn : *contents) {
		auto stroke = atomStrokeDataParse({&tkn, 1uz});
		if (!stroke) return Unexpected(stroke.error());
		strokes.push_back(*stroke);
	}

	auto modifiers = modsStrokeParse(
		Util::subspan(tokens, it, tokens.end())
	);
	if (!modifiers) return Unexpected(modifiers.error());

	return Element {Element::Data, strokes, *modifiers};
}

auto SketchFormat::typeRawParse(TokenSpan tokens)
-> Expected<Element> {
	if (tokens.empty()) return Unexpected(EmptyElement);
	if (tokens[0] != Token {"["}) return Unexpected(MissingBracketLeft);

	auto it = tokens.begin();
	auto contents = parenParse(tokens, it);
	if (!contents) return Unexpected(contents.error(), *it);

	std::vector<Stroke> strokes {};
	strokes.reserve(contents->size());

	for (const Token tkn : *contents) {
		auto stroke = atomStrokeRawParse({&tkn, 1uz});
		if (!stroke) return Unexpected(stroke.error());
		strokes.push_back(*stroke);
	}

	auto modifiers = modsStrokeParse(
		Util::subspan(tokens, it, tokens.end())
	);
	if (!modifiers) return Unexpected(modifiers.error());

	return Element {Element::Data, strokes, *modifiers};
}

auto SketchFormat::typeMarkerParse(TokenSpan tokens)
-> Expected<Element> {
	if (tokens.empty()) return Unexpected(EmptyElement);

	auto marker = atomMarkerParse({tokens.begin(), 1uz});
	if (!marker) return Unexpected(marker.error(), tokens[0]);

	auto modifiers = modsMarkerParse(
		Util::subspan(tokens, ++tokens.begin(), tokens.end())
	);
	if (!modifiers) return Unexpected(modifiers.error(), tokens[0]);

	return Element {Element::Marker, std::vector {*marker}, *modifiers};
}

/* ~~ Atom Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::atomStrokeDataParse(TokenSpan tokens)
-> Expected<Stroke> {
	if (tokens.size() != 1) return Unexpected(AtomSize);

	auto strokeStr = removeTicks(tokens[0]);
	if (!strokeStr) return Unexpected(strokeStr.error(), tokens[0]);
	auto& str = *strokeStr;

	Stroke result {};

	if (str.size()%6 != 0) return Unexpected(StrokeLength, tokens[0]);
	for (std::size_t i=0; i<str.size(); i+=6) {
		auto x = Base36::parse<3,signed>(str.substr(i+0, 3));
		auto y = Base36::parse<3,signed>(str.substr(i+3, 3));
		if (!x || !y) return Unexpected(ForeignDigit, tokens[0]);
		result.points.emplace_back(*x, *y, 1.0);
	}
	return result;
}

auto SketchFormat::atomStrokeRawParse(TokenSpan tokens)
-> Expected<Stroke> {
	if (tokens.size() != 1) return Unexpected(AtomSize);

	auto strokeStr = removeTicks(tokens[0]);
	if (!strokeStr) return Unexpected(strokeStr.error(), tokens[0]);
	auto& str = *strokeStr;

	Stroke result {};

	if (str.size()%4 != 0) return Unexpected(StrokeLength, tokens[0]);
	for (std::size_t i=0; i<str.size(); i+=4) {
		auto x = Base36::parse<2,unsigned>(str.substr(i+0, 2));
		auto y = Base36::parse<2,unsigned>(str.substr(i+2, 2));
		if (!x || !y) return Unexpected(ForeignDigit, tokens[0]);
		result.points.emplace_back(*x, *y, 1.0);
	}
	return result;
}

auto SketchFormat::atomMarkerParse(TokenSpan tokens)
-> Expected<Marker> {
	if (tokens.size() != 1) return Unexpected(AtomSize);
	if (!isStringLiteral(tokens[0])) {
		return Unexpected(MissingString, tokens[0]);
	}

	return Marker {std::string {
		std::next(tokens[0].string.begin()),
		std::prev(tokens[0].string.end())
	}};
}

/* ~~ Modifier Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::modsStrokeParse(TokenSpan tokens)
-> Expected<StrokeModifiers> {
	StrokeModifiers result {};

	for (auto it = tokens.begin(); it != tokens.end(); /**/) {
		Parser<Mod::Of_Stroke>* modParser =
		  *it == Token {"Affine"} ? modAffineParse
		: *it == Token {"Array" } ? modArrayParse
		: /*                    */   nullptr;
		if (!modParser) return Unexpected(UnknownModifierType, *it);

		auto contents = parenParse(tokens, ++it);
		if (!contents) return Unexpected(contents.error(), *it);

		auto modifier = modParser(*contents);
		if (!modifier) return Unexpected(modifier.error(), *it);

		result.push_back(*modifier);
	}

	return result;
}

auto SketchFormat::modsMarkerParse(TokenSpan tokens)
-> Expected<MarkerModifiers> {
	MarkerModifiers result {};

	for (auto it = tokens.begin(); it != tokens.end(); /**/) {
		Parser<Mod::Of_Marker>* modParser =
		  *it == Token {"Uppercase"} ? modUppercaseParse
		: /*                       */   nullptr;
		if (!modParser) return Unexpected(UnknownModifierType, *it);

		auto contents = parenParse(tokens, ++it);
		if (!contents) return Unexpected(contents.error(), *it);

		auto modifier = modParser(*contents);
		if (!modifier) return Unexpected(modifier.error(), *it);

		result.push_back(*modifier);
	}

	return result;
}

auto SketchFormat::modAffineParse(TokenSpan tokens)
-> Expected<Mod::Of_Stroke> {
	if (tokens.size() != 9) return Unexpected(ModAffineSize);

	std::array<double,9> matrix {};
	for (std::size_t i=0; i<9; i++) {
		auto number = floatParse<double>(tokens[i]);
		if (!number) return Unexpected(number.error(), tokens[i]);
		matrix[i] = *number;
	}

	return Mod::Affine {matrix};
}

auto SketchFormat::modArrayParse(TokenSpan tokens)
-> Expected<Mod::Of_Stroke> {
	if (tokens.size() != 3+9+1) return Unexpected(ModArraySize);

	auto n = integerParse<std::size_t>(tokens[0]);
	if (!n) return Unexpected(n.error(), tokens[0]);

	if (tokens[ 1] != Token {"Affine"}
	||  tokens[ 2] != Token {"["}
	||  tokens[12] != Token {"]"}) {
		return Unexpected(MalformedModArray, tokens[0]);
	}
	auto affine = modAffineParse(tokens.subspan(3uz, 9uz));
	if (!affine) return Unexpected(affine.error(), tokens[1]);

	return Mod::Array {*n, std::get<Mod::Affine>(*affine)};
}

auto SketchFormat::modUppercaseParse(TokenSpan tokens)
-> Expected<Mod::Of_Marker> {
	if (tokens.size() != 0) return Unexpected(ModUppercaseSize);
	return Mod::Uppercase {};
}

/* ~~ Helper Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::parenParse(TokenSpan tokens, TokenIter& it)
-> Expected<TokenSpan> {
	if (isStringLiteral(*it)) return TokenSpan {it++, 1uz};

	constexpr std::array parenTypes {
		std::pair {Token {"["}, Token {"]"}},
		/* reserved for future types */
	};

	struct Paren { std::size_t id; enum { Left, Right } side; };
	auto parenTypeOf = [&](Token tkn) -> std::optional<Paren> {
		for (std::size_t i=0; i<parenTypes.size(); i++) {
			auto [left,right] = parenTypes[i];
			if (tkn == left ) return Paren {i, Paren::Left };
			if (tkn == right) return Paren {i, Paren::Right};
		}
		return std::nullopt;
	};

	if (auto t = parenTypeOf(*it); !t || t->side != Paren::Left) {
		return Unexpected(MissingBracketLeft, *it);
	}

	std::stack<Paren> stack {};
	const auto start = it;

	do if (auto t = parenTypeOf(*it++)) {
		if (t->side == Paren::Left ) stack.push(*t);
		if (t->side == Paren::Right) {
			if (t->id != stack.top().id) {
				return Unexpected(MismatchingParens, *it);
			}
			stack.pop();
		}
	} while (!stack.empty() && it!=tokens.end());

	TokenSpan result {Util::subspan(tokens,
		std::next(start),
		std::prev(it)
	)};
	if (stack.empty()) return result;

	return Unexpected(MissingBracketRight, tokens.back());
}

auto SketchFormat::isStringLiteral(const Token tkn) -> bool {
	return tkn.string.size() >= 2
	&&     tkn.string.front() == '('
	&&     tkn.string.back()  == ')';
}

auto SketchFormat::removeTicks(const Token tkn) -> Expected<std::string> {
	std::string result {tkn.string};
	constexpr char Tick = '\'';

	if (!result.empty()
	&& (result.front() == Tick
	||  result.back() == Tick
	||  result.contains(std::string(2, Tick)))) {
		return Unexpected(TickmarkOrdering, tkn);
	}

	auto isNotTick = [](char c) { return c != Tick; };

	return result
		| views::filter(isNotTick)
		| views::transform(Util::toLower)
		| ranges::to<std::string>();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

std::ostream& operator<<(
	std::ostream& os, const ParserBase::SourcePos& pos
) {
	if (pos.row == 0) return os << "[Unknown Position]";
	os << pos.row;
	if (pos.col > 0) os << ":" << pos.col;
	return os;
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
		std::cout << "\tError code:  " << (int)tokens.error() << "\n";
		std::cout << "\tAt Position: " << tokens.error().pos << "\n";
	}
}