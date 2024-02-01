#include "types.hh"
#include "util.hh"
#include <vector>
#include <span>

namespace
{
	using namespace Atom;

	auto flattenStrokes(std::span<const Stroke> strokes) {
		std::vector<FlatStroke> result {};
		result.reserve(strokes.size());

		for (const Stroke& s : strokes) {
			FlatStroke subResult {};
			for (const auto& p : s.points) {
				// Ignore p.pressure ... (for now).
				subResult.points.emplace_back(p.x, p.y);
			}
			result.push_back(std::move(subResult));
		}

		return result;
	}
}

/* ~~ Main "Flatten" Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto Sketch::render() const -> FlatSketch {
	FlatSketch result {};
	auto append = [&result](ranges::input_range auto&& r) {
		ranges::copy(r, std::back_inserter(result.strokes));
	};

	for (const auto& elem : elements) {
		std::visit(Util::Overloaded {
			[&](const Brush&  e) { append(flattenStrokes(e.atoms)); },
			[&](const Pencil& e) { append(e.atoms); },
			[&](const Data&   e) { append(e.atoms); },
			[&](const auto&) {},
		}, elem);
	}

	return result;
}