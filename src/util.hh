#pragma once

namespace Util
{
	template <typename... Ts>
	struct Overloaded : Ts... { using Ts::operator()...; };

	template <typename T, typename... Ts>
	static constexpr bool isAny(T x, Ts... args) {
		auto compare = [](T x, auto arg) {
			// Specialization for looking up chars in a string:
			if constexpr (std::is_convertible_v<decltype(arg), std::string_view>)
				return std::string_view{arg}.find(x) != std::string_view::npos;
			else
				return x == arg;
		};

		return (compare(x,args) || ... );
	}
}