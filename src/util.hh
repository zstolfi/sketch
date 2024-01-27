#pragma once
#include <algorithm>
#include <ranges>
#include <span>
namespace ranges = std::ranges;
namespace views = std::views;
using namespace std::literals;

namespace Util
{
	// Helper struct for std::visit and std::apply
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

	// Template-argument ready string literal
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

	// Generic repeated multiplication
	template <typename T>
	constexpr T pow(T x, std::size_t y) {
		T result (1);
		while (y--) result = result * x;
		return result;
	}

	// Alternate for ranges::contains unitl clang supports it
	template <ranges::input_range R, typename T>
	constexpr bool contains(R&& r, const T& value) {
		return ranges::find(r, value) != ranges::end(r);
	}

	// Create a subspan from iterators
	template <typename T>
	constexpr std::span<T> subspan(
		const std::span<T>& span,
		typename std::span<T>::iterator begin,
		typename std::span<T>::iterator end
	) {
		return span.subspan(
			std::distance(span.begin(), begin),
			std::distance(begin, end)
		);
	}

	// Create a subspan of const from iterators
	template <typename T>
	constexpr std::span<const T> subspan(
		const std::span<const T>& span,
		typename std::span<const T>::iterator begin,
		typename std::span<const T>::iterator end
	) {
		return span.subspan(
			std::distance(span.begin(), begin),
			std::distance(begin, end)
		);
	}

	// Closer of 2 iterators
	template <std::contiguous_iterator T>
	constexpr T min(T a, T b) {
		return (std::distance(a,b) > 0) ? a : b;
	}

	// Further of 2 iterators
	template <std::contiguous_iterator T>
	constexpr T max(T a, T b) {
		return (std::distance(a,b) < 0) ? a : b;
	}

	// Character modifiers
	constexpr char toUpper(char c) { return std::toupper(c); }
	constexpr char toLower(char c) { return std::tolower(c); }
}