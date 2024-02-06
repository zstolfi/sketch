#include "parsers.hh"
#include "util.hh"
#include <string>
#include <stack>
#include <optional>

/* ~~ Raw Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto RawFormat::parse(std::string_view str)
-> Expected<FlatSketch> {
	return rawParse(tokenize(str));
}

auto RawFormat::tokenize(std::string_view str)
-> std::vector<Token> {
	std::vector<Token> result {};
	for (std::size_t i=0, j=0; i<str.size(); i=j) {
		while (i < str.size() && isWhitespace(str[i])) i++;
		if (i == str.size()) break;
		j = i;
		while (j < str.size() && !isWhitespace(str[j])) j++;
		result.emplace_back(str.substr(i, j-i));
	}

	return result;
}

auto RawFormat::rawParse(TokenSpan tokens)
-> Expected<FlatSketch> {
	FlatSketch result {};

	for (Token tkn : tokens) {
		Atom::FlatStroke stroke {};
		auto points = Base36::parseTuples(
			tkn.string,
			+[](Base36::Number_t<2,unsigned> x
			,   Base36::Number_t<2,unsigned> y) {
				return Atom::FlatStroke::Point {int(*x), int(*y)};
			}
		);
		if (!points) return Unexpected(MalformedNumberTuple, tkn);
		
		result.strokes.emplace_back(*points);
	}

	return result;
}

void RawFormat::print(std::ostream& os, const FlatSketch& sketch) {
	for (const auto& s : sketch.strokes) {
		os << " ";
		for (const auto& p : s.points) {
			os << Base36::toString<2,unsigned>(p.x)
			   << Base36::toString<2,unsigned>(p.y);
		}
	}
}

/* ~~ Sketch Parser ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::parse(std::string_view str)
-> Expected<Sketch> {
	auto tokens = tokenize(str);
	if (!tokens) return Unexpected(tokens.error());
	return sketchParse(*tokens);
}

void SketchFormat::print(std::ostream& os, const Sketch& s) {
	os << s;
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
	  tokens[0] == Token {"Brush" } ? typeBrushParse
	: tokens[0] == Token {"Pencil"} ? typePencilParse
	: tokens[0] == Token {"Data"  } ? typeDataParse
	: tokens[0] == Token {"Raw"   } ? typeRawParse
	: tokens[0] == Token {"Marker"} ? typeMarkerParse
	: /* default:                */   nullptr;
	if (!elemParser) return Unexpected(UnknownElementType, tokens[0]);

	return elemParser(
		Util::subspan(tokens, ++tokens.begin(), tokens.end())
	);
}

/* ~~ Element Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::typeBrushParse(TokenSpan tokens)
-> Expected<Element>
{ return parseStroke<Brush, 2>(tokens, atomStrokeBrushParse); }

auto SketchFormat::typePencilParse(TokenSpan tokens)
-> Expected<Element>
{ return parseStroke<Pencil, 1>(tokens, atomStrokeDataParse); }

auto SketchFormat::typeDataParse(TokenSpan tokens)
-> Expected<Element>
{ return parseStroke<Data, 1>(tokens, atomStrokeDataParse); }

auto SketchFormat::typeRawParse(TokenSpan tokens)
-> Expected<Element>
{ return parseStroke<Data, 1>(tokens, atomStrokeRawParse); }

auto SketchFormat::typeMarkerParse(TokenSpan tokens)
-> Expected<Element> {
	if (tokens.empty()) return Unexpected(EmptyElement);

	auto marker = atomMarkerParse({tokens.begin(), 1uz});
	if (!marker) return Unexpected(marker.error(), tokens[0]);

	auto modifiers = modsMarkerParse(
		Util::subspan(tokens, ++tokens.begin(), tokens.end())
	);
	if (!modifiers) return Unexpected(modifiers.error(), tokens[0]);

	return Marker {*marker, *modifiers};
}

/* ~~ Atom Parsers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto SketchFormat::atomStrokeBrushParse(TokenSpan tokens)
-> Expected<Atom::Stroke> {
	if (tokens.size() != 2) return Unexpected(AtomSize);

	auto diameter = Base36::parse<2,unsigned>(tokens[0].string);
	if (!diameter) return Unexpected(ForeignDigit, tokens[0]);

	auto strokeStr = removeTicks(tokens[1].string);
	if (!strokeStr) return Unexpected(strokeStr.error(), tokens[1]);

	auto points = Base36::parseTuples(
		*strokeStr,
		+[](Base36::Number_t<3,  signed> x
		,   Base36::Number_t<3,  signed> y
		,   Base36::Number_t<2,unsigned> p) {
			return Atom::Stroke::Point {*x, *y, *p/double(36*36-1)};
		}
	);
	if (!points) return Unexpected(MalformedNumberTuple, tokens[1]);

	return Atom::Stroke {*diameter, std::move(*points)};
}

auto SketchFormat::atomStrokeDataParse(TokenSpan tokens)
-> Expected<Atom::FlatStroke> {
	if (tokens.size() != 1) return Unexpected(AtomSize);

	auto strokeStr = removeTicks(tokens[0].string);
	if (!strokeStr) return Unexpected(strokeStr.error(), tokens[0]);
	
	auto points = Base36::parseTuples(
		*strokeStr,
		+[](Base36::Number_t<3,signed> x
		,   Base36::Number_t<3,signed> y) {
			return Atom::FlatStroke::Point {*x, *y};
		}
	);
	if (!points) return Unexpected(MalformedNumberTuple, tokens[0]);

	return Atom::FlatStroke {std::move(*points)};
}

auto SketchFormat::atomStrokeRawParse(TokenSpan tokens)
-> Expected<Atom::FlatStroke> {
	if (tokens.size() != 1) return Unexpected(AtomSize);

	auto strokeStr = removeTicks(tokens[0].string);
	if (!strokeStr) return Unexpected(strokeStr.error(), tokens[0]);
	
	auto points = Base36::parseTuples(
		*strokeStr,
		+[](Base36::Number_t<2,unsigned> x
		,   Base36::Number_t<2,unsigned> y) {
			return Atom::FlatStroke::Point {signed(*x), signed(*y)};
		}
	);
	if (!points) return Unexpected(MalformedNumberTuple, tokens[0]);

	return Atom::FlatStroke {std::move(*points)};
}

auto SketchFormat::atomMarkerParse(TokenSpan tokens)
-> Expected<Atom::Marker> {
	if (tokens.size() != 1) return Unexpected(AtomSize);
	if (!isStringLiteral(tokens[0])) {
		return Unexpected(MissingString, tokens[0]);
	}

	return Atom::Marker {std::string {
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

auto SketchFormat::removeTicks(std::string_view str)
-> Expected<std::string> {
	std::string result {str};
	constexpr char Tick = '\'';

	if (!result.empty()
	&& (result.front() == Tick
	||  result.back() == Tick
	||  result.contains(std::string(2, Tick)))) {
		return Unexpected(TickmarkOrdering);
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
		std::cout << "\tError code:  " << int(tokens.error()) << "\n";
		std::cout << "\tAt Position: " << tokens.error().pos << "\n";
	}
}