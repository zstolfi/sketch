#pragma once
// Header file for ANY base (but 36 and 10 mainly)
#include "util.hh"
#include <expected>
#include <string>
#include <string_view>
#include <concepts>

template <std::size_t B, Util::FixedString Alphabet>
struct Base {
	static_assert(B > 1);
	static_assert(ranges::size(Alphabet.data) >= B);
	static_assert([]() {
		// Check for digit uniqueness
		for (char c : Alphabet.data) {
			if (ranges::count(Alphabet.data, c) != 1) return false;
		}
		return true;
	} ());

	enum parseError {
		ForeignDigit,
		StringSize,
	};

	static auto isDigit(char c) {
		bool result {};
		// result = ranges::contains(Alphabet.data, c);
		result = ranges::find(Alphabet.data, c)
		!=       ranges::end(Alphabet.data);
		return result;
	}

	static auto digitValue(char c) {
		std::size_t result {};
		result = std::distance(
			ranges::begin(Alphabet.data),
			ranges::find(Alphabet.data, c)
		);
		return result;
	}

	template <std::size_t N, std::integral T>
	static auto parse(std::string_view str)
	-> std::expected<T,parseError> {
		T result {};
		if (str.size() != N) return std::unexpected(StringSize);

		for (char c : str) {
			if (!isDigit(c)) return std::unexpected(ForeignDigit);
			result = B * result + digitValue(c);
		}
		if (std::is_signed_v<T>) {
			// Parse signed bases in a 2's-complement style
			constexpr T topBase = Util::pow(B,N);
			// ~~~~~~ Examples: ~~~~~~
			// B=16, N=3
			//     000 7ff -> +0 +2047
			//     800 fff -> -2048 -1
			// B=3, N=4
			//     0000 1111 -> +0 +40
			//     1112 2222 -> -40 -1

			// Arguably odd signed bases are nicer, because each
			// negative necesarily has a corresponding positive.
			constexpr T rollover = topBase/2 + (B&1);
			if (result >= rollover) result -= topBase;
		}

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