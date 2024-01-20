#pragma once
// Header file for ANY base (but 36 and 10 mainly)
#include <expected>
#include <string_view>
#include <concepts>
#include <cassert>

template <std::size_t B>
struct Base {
	static_assert(N > 1);

	enum struct parseError {
		/* ... */
	};

	static auto isDigit(char c) {
		bool result {};
		/* ... */
		return result;
	}

	template <std::size_t N, std::integral T>
	static auto parse(std::string_view str) {
		std::expected<T,parseError> result {};
		/* ... */
		return result;
	}

	template <std::size_t N, std::integral T>
	static auto toString(T) {
		std::string result {};
		/* ... */
		return result;
	}
};

// Aliases just to be sure that I don't
// accidentally type the wrong numbers.
using Base10 = Base<10>;
using Base36 = Base<36>;