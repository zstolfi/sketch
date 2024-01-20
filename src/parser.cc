#include "parser.hh"
#include "util.hh"
#include <ranges>
namespace ranges = std::ranges;

bool RawFormat::verify(std::istream& is) {
	enum { S0, X1, X2, Y1, Y2 } state = S0;
	for (char c; is.get(c); )
		if (isBase36(c))
			state = (state == S0) ? X1
			:       (state == X1) ? X2
			:       (state == X2) ? Y1
			:       (state == Y1) ? Y2
			:     /*(state == Y2)*/ X1;
		else if (isWhitespace(c) && (state == S0 || state == Y2))
			state = S0;
		else
			return false;
	return state == S0 || state == Y2;
}

RawSketch RawFormat::parse(std::istream& is) {
	RawSketch result {};
	for (std::string line; is >> std::ws >> line; ) {
		RawStroke stroke {};
		for (std::size_t i=0; i<line.size(); i+=4) {
			RawPoint point {};
			point.x = base36<3,int16_t>(line.substr(i+0, 2));
			point.y = base36<3,int16_t>(line.substr(i+2, 2));
			stroke.points.push_back(point);
		}
		result.strokes.push_back(stroke);
	}
	return result;
}

// Removes whitespace/comments.
SketchFormat::Tokens SketchFormat::tokenize(std::string_view str) {
	Tokens result;
	auto resultAdd = [&](std::size_t i0, std::size_t i1) {
		result.push_back(str.substr(i0, i1-i0));
	};

	// Token includes types/numbers/base36.
	// Op is any single character operator.
	enum {
		LineStart, Comment, Space, End,
		Token, String, StringEnd, Op
	}
	prevState = LineStart,
	nextState;

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
			else /*            */ return Token;
		} (prevState, str[i]);

		if (prevState == Op) {
			resultAdd(i-1, i);
			if (str[i] == ';') return result;
		}

		if (prevState != nextState) {
			if (nextState == Token) tokenStart = i;
			if (prevState == Token) resultAdd(tokenStart, i);

			if (nextState == String) tokenStart=i, parenCount=1;
			if (nextState == StringEnd) resultAdd(tokenStart, i+1);

			// The source file ends with an unterminated string
			// literal. Which is probably because of unbalanced
			// parentheses in the string. The tokenizer doesn't
			// error because the validity can be checked later.
			if (prevState == String && nextState == End)
				resultAdd(tokenStart, i);
		}
	}
	return result;
}

// TODO: change std::optional to std::expected
auto SketchFormat::parseElement(const Tokens& tkn, std::size_t& i)
-> std::optional<ElementData> {
	if (i+1 > tkn.size()) return {};
	Token typeName = tkn[i++];

	auto match = ranges::find_if(ElementsDefs,
		[&](auto t) { return t.first == typeName; }
	);
	if (match == ElementsDefs.end()) return {};

	using Ret_t = std::optional<ElementData>;
	return std::visit(Util::Overloaded {
		[&](tNone v) -> Ret_t {
			return ElementData {typeName, {}};
		},
		[&](tSingle v) -> Ret_t {
			if (i+2 > tkn.size()) return {};
			if (tkn[i++] != ":") return {};
			return ElementData {typeName, {&tkn[i++], 1}};
		},
		[&](tBounded v) -> Ret_t {
			if (i+2 > tkn.size()) return {};
			if (tkn[i++] != ":") return {};
			if (tkn[i++] != "[") return {};
			std::size_t count = 0;
			while (i < tkn.size() && tkn[i] != "]")
				i++, count++;
			if (i++ == tkn.size()) return {};
			if (count != v.n) return {};
			return ElementData {typeName, {&tkn[i-1-count], count}};
		},
		[&](tUnbounded v) -> Ret_t {
			if (i+2 > tkn.size()) return {};
			if (tkn[i++] != ":") return {};
			if (tkn[i++] != "[") return {};
			std::size_t count = 0;
			while (i < tkn.size() && tkn[i] != "]")
				i++, count++;
			if (i++ == tkn.size()) return {};
			return ElementData {typeName, {&tkn[i-1-count], count}};
		}
	}, match->second);
}

auto SketchFormat::parse(const Tokens& tkn)
-> std::optional<Sketch> {
	if (tkn.empty()) return {};
	if (tkn[0] == ";") return Sketch {};

	Sketch result {};
	for (std::size_t i=0; i<tkn.size(); i++) {
		std::vector<ElementData> elemsList {};
		while (i<tkn.size() && !Util::isAny(tkn[i], ",", ";")) {
			if (auto e = parseElement(tkn, i))
				elemsList.push_back(*e);
			else
				return {};
		}

		// All 'statements' must contain > 0 elements.
		if (elemsList.empty()) return {};
		auto currElem = elemsList.begin();

		std::vector<Atom> timelineAtoms {};
		Element timelineElem {ElementType::Data, {}};

		bool isGrouping = false;
		if (auto it = elementTypeFromString.find(currElem->type)
		;   it != elementTypeFromString.end()) {
			isGrouping = true;
			timelineElem.type = it->second;
		}

		/* PARSE MAIN ELEMENT */
		if (Util::isAny(currElem->type, "Data", "Pencil", "Brush")) {
			const bool isBrush = currElem->type == "Brush";
			for (std::size_t j=0; j<currElem->members.size(); /**/) {
				unsigned diameter = isBrush
					? base36<2,unsigned>(currElem->members[j++])
					: 3;
				Stroke stroke {diameter, {}};
				std::string digits {};
				for (char c : currElem->members[j++]) {
					if (c == '\'') continue;
					digits.push_back(c);
					if (digits.size() < (isBrush? 8:6)) continue;
					stroke.points.push_back(Point {
						base36<3,int>(digits.substr(0, 3)),
						base36<3,int>(digits.substr(3, 3)),
						isBrush
							? base36<2,unsigned>(digits.substr(6,2))
								/ float(36*36-1)
							: 1.0f
					});
					digits.clear();
				}
				assert(digits.empty());
				timelineAtoms.push_back(stroke);
			}
		}
		else if (currElem->type == "Marker") {
			std::string_view message = currElem->members[0];
			assert(message.starts_with('(')
			&&     message.ends_with  (')'));
			message.remove_prefix(1), message.remove_suffix(1);
			timelineAtoms.push_back(
				Marker {std::string {message}}
			);
		}
		++currElem;

		/* PARSE ALL MODIFIERS */
		for (; currElem != elemsList.end(); ++currElem) {
			if (currElem->type == "Affine") {
				std::array<float,9> m;
				for (std::size_t j=0; j<9; j++)
					m[j] = base10<float>(currElem->members[j]);

				timelineElem.modifiers.push_back(Mod::Affine {m});
			}
		}

		if (isGrouping) {
			timelineElem.atoms = {
				timelineAtoms.begin(),
				timelineAtoms.end()
			};
			result.elements.push_back(timelineElem);
		}
		
		if (i<tkn.size() && tkn[i] == ";") break;
	}
	return result;
}