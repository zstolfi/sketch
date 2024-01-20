#pragma once
// Header file for ANY base (but 36 and 10 mainly)
#include "util.hh"
#include <expected>
#include <string>
#include <string_view>
#include <concepts>
#include <cassert>

template <std::size_t B, Util::FixedString Alphabet>
struct Base {
	static_assert(B > 1);
	// Each alphabet char must be unique
	static_assert([]() {
		for (char c : Alphabet.data) {
			if (ranges::count(Alphabet.data, c) != 1) return false;
		}
		return true;
	} ());

	enum struct parseError {
		/* ... */
	};

	static auto isDigit(char c) {
		bool result {};
		// result = ranges::contains(Alphabet.data, c);
		result = ranges::any_of(Alphabet.data, c);
		return result;
	}

	template <std::size_t N, std::integral T>
	static auto parse(std::string_view str) {
		std::expected<T,parseError> result {};
		assert(str.size() == N); // Might change to <= later
		/* ... */
		return result;
	}

	template <std::size_t N, std::integral T>
	static auto toString(T x) {
		std::string result (N, Alphabet.data[0]);
		for (std::size_t i=0; i<N; i++) {
			result[N-i-1] = Alphabet.data[x % B];
			x /= B;
		}
		return result;
	}
};

using Base10 = Base<10,"0123456789">;
using Base36 = Base<36,"0123456789abcdefghijklmnopqrstuvwxyz">;