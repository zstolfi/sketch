#pragma once
#include "parserBase.hh"
#include "types.hh"
#include "math.hh"
#include <iostream>
#include <vector>
#include <variant>

class RawFormat : public ParserBase {
public:
	// Alternatively I could use an istream& instead of a string_view.
	static auto parse(std::string_view) -> Expected<FlatSketch>;
	static void print(std::ostream&, const FlatSketch&);

private:
	static auto tokenize(std::string_view)
	-> std::vector<Token>;

	static auto rawParse(TokenSpan)
	-> Expected<FlatSketch>;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

class SketchFormat : public ParserBase {
public:
	static auto parse(std::string_view) -> Expected<Sketch>;
	static void print(std::ostream&, const Sketch&);

private:
	static auto tokenize(std::string_view)
	-> Expected<std::vector<Token>>;

	static auto parenParse(TokenSpan, TokenIter&)
	-> Expected<TokenSpan>;

	static auto isStringLiteral(const Token) -> bool;
	static auto removeTicks(std::string_view) -> Expected<std::string>;

	template <typename T, typename... Args>
	using Parser = auto (TokenSpan, Args...) -> Expected<T>;

	static Parser  <Sketch>                         sketchParse;
	static Parser/*└─*/<Element>                    elementParse;
	static Parser/*    │ */<Element>                typeBrushParse;
	static Parser/*    │ */<Element>                typePencilParse;
	static Parser/*    │ */<Element>                typeDataParse;
	static Parser/*    │ */<Element>                typeRawParse;
	static Parser/*    │ */<Element>                typeMarkerParse;
	/*                 ├── <Atoms> */
	static Parser/*    │     */<Atom::Stroke>       atomStrokeBrushParse;
	static Parser/*    │     */<Atom::FlatStroke>   atomStrokePencilParse;
	static Parser/*    │     */<Atom::FlatStroke>   atomStrokeDataParse;
	static Parser/*    │     */<Atom::FlatStroke>   atomStrokeRawParse;
	static Parser/*    │     */<Atom::Marker>       atomMarkerParse;
	/*                 └── <Modifiers> */
	static Parser/*        ├─*/<StrokeModifiers>    modsStrokeParse;
	static Parser/*        │     */<Mod::Of_Stroke> modAffineParse;
	static Parser/*        │     */<Mod::Of_Stroke> modArrayParse;
	static Parser/*        └─*/<MarkerModifiers>    modsMarkerParse;
	static Parser/*              */<Mod::Of_Marker> modUppercaseParse;

	template <typename E, std::size_t N, typename Atom_t>
	static Parser<Element, Parser<Atom_t>&> parseStroke;

public:
	static void printTokens(std::string_view);
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

template <typename Element_t, std::size_t AtomLen, typename Atom_t>
auto SketchFormat::parseStroke(
	TokenSpan tokens,
	Parser<Atom_t>& atomParser
)
-> Expected<Element> {
	if (tokens.empty()) return Unexpected(EmptyElement);
	if (tokens[0] != Token {"["}) return Unexpected(MissingBracketLeft);

	auto it = tokens.begin();
	auto contents = parenParse(tokens, it);
	if (!contents) return Unexpected(contents.error(), *it);

	std::vector<Atom_t> strokes {};
	strokes.reserve(contents->size());

	if (tokens.size() % AtomLen) return Unexpected(ElementBrushSize);
	for (auto jt=contents->begin(); jt!=contents->end()
	;    std::advance(jt, AtomLen)) {
		auto stroke = atomParser({jt, AtomLen});
		if (!stroke) return Unexpected(stroke.error());
		strokes.push_back(*stroke);
	}

	auto modifiers = modsStrokeParse(
		Util::subspan(tokens, it, tokens.end())
	);
	if (!modifiers) return Unexpected(modifiers.error());

	return Element_t {strokes, *modifiers};
}