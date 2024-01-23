#pragma once
#include <algorithm>
#include <ranges>
#include <span>
namespace ranges = std::ranges;
namespace views = std::views;
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
		constexpr std::size_t size() const { return N-1; }
		constexpr const char* begin() const { return &data[0]; }
		constexpr const char* end  () const { return &data[N-1]; }
	};

	template <typename T>
	constexpr T pow(T x, std::size_t y) {
		T result {1};
		while (y--) result *= x;
		return result;
	}

	// Alternate for ranges::contains unitl clang supports it
	template <ranges::input_range R, typename T>
	constexpr bool contains(R&& r, const T& value) {
		return ranges::find(r, value) != ranges::end(r);
	}

	// Create a subspan from iterators
	template <typename T>
	constexpr std::span<T, std::dynamic_extent> subspan(
		const std::span<T>& span,
		T::iterator begin,
		T::iterator end
	) {
		return span.subspan(
			std::distance(span.begin(), begin),
			std::distance(begin, end)
		);
	}

	template <typename T>
	constexpr std::span<const T, std::dynamic_extent> subspan(
		const std::span<const T>& span,
		T::const_iterator cbegin,
		T::const_iterator cend
	) {
		return span.subspan(
			std::distance(span.cbegin(), cbegin),
			std::distance(cbegin, cend)
		);
	}

	template <std::continguous_iterator T>
	constexpr T min(T a, T b) {
		return (std::distance(a,b) > 0) ? a : b;
	}
}