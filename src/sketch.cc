#include "types.hh"
#include "util.hh"
#include <vector>
#include <span>

namespace
{
	using namespace Atom;

	auto renderStroke(const Stroke s) {
		FlatStroke result {};
		for (const auto& p : s.points) {
			// Ignore p.pressure ... (for now).
			result.points.emplace_back(p.x, p.y);
		}
		return result;
	}

	auto renderStrokes(std::span<const Stroke> strokes) {
		return strokes
			| views::transform(renderStroke)
			| ranges::to<std::vector>();
	}

	auto strokeModsReduce(StrokeModifiers mods) -> StrokeModifiers {
		// Reduce all adjacent affine modifiers to lessen
		// computation on strokes, also to hold floating-
		// point -> int rounding for as long as possible.
		if (mods.size() < 2) return mods;
		for (auto it = mods.begin(), next = it
		;    it != mods.end(); it = next) {
			next = std::next(it);
			auto* left  = std::get_if<Mod::Affine>(&*it);
			auto* right = std::get_if<Mod::Affine>(&*next);
			if (!left || !right) continue;

			*right = *left * *right;
			next = mods.erase(it);
		}
		return mods;
	}
}

/* ~~ Main "Flatten" Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto Sketch::render() const -> FlatSketch {
	FlatSketch result {};
	auto append = [](auto& v, ranges::input_range auto&& r) {
		ranges::copy(r, std::back_inserter(v));
	};

	for (const auto& variant : elements) {
		std::visit([&]<typename T>(const T& e) {
			if constexpr (HoldsStrokeMods<T>) {
				auto mods = strokeModsReduce(e.modifiers);
				std::vector<Stroke> strokes {std::from_range, e.atoms};

				for (const auto& variant : mods) {
					std::visit([&strokes](const auto& mod) {
						strokes = mod(strokes);
					}, variant);
				}

				append(result.strokes, renderStrokes(strokes));
			}
		}, variant);
	}

	return result;
}