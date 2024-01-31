#pragma once
#include "parserBase.hh"
#include "types.hh"
#include "math.hh"
#include <iostream>
#include <vector>
#include <variant>

class RawFormat : public ParserBase {
public:
	static auto parse(std::string_view) -> Expected<RawSketch>;

private:
	static auto tokenize(std::string_view)
	-> std::vector<Token>;

	static auto rawParse(TokenSpan)
	-> Expected<RawSketch>;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

class SketchFormat : public ParserBase {
public:
	static auto parse(std::string_view) -> Expected<Sketch>;

private:
	static auto tokenize(std::string_view)
	-> Expected<std::vector<Token>>;

	static auto parenParse(TokenSpan, TokenIter&)
	-> Expected<TokenSpan>;

	static auto isStringLiteral(const Token) -> bool;
	static auto removeTicks(std::string_view) -> Expected<std::string>;

	template <typename T>
	using Parser = auto (TokenSpan) -> Expected<T>;

	static Parser   <Sketch>                        sketchParse;
	static Parser /*└─*/<Element>                   elementParse;
	static Parser /*    │ */<Element>               typeBrushParse;
	static Parser /*    │ */<Element>               typePencilParse;
	static Parser /*    │ */<Element>               typeDataParse;
	static Parser /*    │ */<Element>               typeRawParse;
	static Parser /*    │ */<Element>               typeMarkerParse;
	              /*    ├── <Atoms> */
	static Parser /*    │     */<Stroke>            atomStrokeBrushParse;
	static Parser /*    │     */<FlatStroke>        atomStrokeDataParse;
	static Parser /*    │     */<FlatStroke>        atomStrokeRawParse;
	static Parser /*    │     */<Marker>            atomMarkerParse;
	static Parser /*    ├─*/<StrokeModifiers>       modsStrokeParse;
	static Parser /*    │     */<Mod::Of_Stroke>    modAffineParse;
	static Parser /*    │     */<Mod::Of_Stroke>    modArrayParse;
	static Parser /*    └─*/<MarkerModifiers>       modsMarkerParse;
	static Parser /*          */<Mod::Of_Marker>    modUppercaseParse;

public:
	static auto printTokens(std::string_view) -> void;
};