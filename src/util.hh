#pragma once
#include <algorithm>
namespace ranges = std::ranges;
using namespace std::literals;

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
			ranges::copy(str, ranges::begin(data));
		}

		constexpr bool operator<=>(const FixedString&) const = default;
		constexpr char operator[](std::size_t i) const { return data[i]; }
		constexpr std::size_t size() const { return N; }
		constexpr const char* begin() const { return &data[0]; }
		constexpr const char* end  () const { return &data[N]; }
	};

	template <typename T>
	constexpr T pow(T x, std::size_t y) {
		T result {1};
		while (y--) result *= x;
		return result;
	}
}