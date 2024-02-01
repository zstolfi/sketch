#include "types.hh"

/* ~~ Main "Flatten" Function ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

auto Sketch::render() -> FlatSketch {
	FlatSketch result ({
		Atom::FlatStroke ({
			{0,0}, {800,0}, {800,600}, {0,600}, {0,0}
		})
	});

	// auto toRawPoint = [](const auto& p) {
	// 	return RawPoint {p.x,p.y};
	// };

	// auto toRawStroke = [&](const auto& s) {
	// 	return RawStroke {
	// 		s.points
	// 		| views::transform(toRawPoint)
	// 		| ranges::to<std::vector>()
	// 	};
	// };

	// for (const Element& e : elements) switch (e.type) {
	// 	case Element::Pencil:
	// 	case Element::Data:
	// 	{
	// 		const auto& strokes = std::get<FlatStrokeAtoms>(e.atoms);
	// 		ranges::copy(
	// 			strokes
	// 			| views::transform(toRawStroke)
	// 			, std::back_inserter(result.strokes)
	// 		);
	// 		break;
	// 	}
	// 	case Element::Brush:
	// 	{
	// 		const auto& strokes = std::get<StrokeAtoms>(e.atoms);
	// 		ranges::copy(
	// 			strokes
	// 			| views::transform(toRawStroke)
	// 			, std::back_inserter(result.strokes)
	// 		);
	// 		break;
	// 	}
	// 	default: break;
	// }
	return result;
}