#pragma once
#include <algorithm>
namespace ranges = std::ranges;

namespace Util
{
	template <typename... Ts>
	struct Overloaded : Ts... { using Ts::operator()...; };

	template <typename T, typename... Ts>
	static constexpr bool isAny(T x, Ts... args) {
		auto compare = [](T x, auto arg) {
			// Specialization for looking up chars in a string:
			if constexpr (requires { std::string_view {arg}; }) {
				return (std::string_view {arg}).contains(x);
			}
			else {
				return x == arg;
			}
		};

		return (compare(x,args) || ... );
	}

	template <std::size_t N>
	struct FixedString {
		char data[N];

		constexpr FixedString(const char (&str)[N]) {
			ranges::copy(str, &data[0]);
		}

		constexpr bool operator<=>(const FixedString&) const = default;
	};
}