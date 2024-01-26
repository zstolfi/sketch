#pragma once
// Header file for ANY base (but 36 and 10 mainly)
#include "util.hh"
#include <limits>
#include <expected>
#include <string>
#include <string_view>
#include <concepts>

template <std::size_t B, Util::FixedString Alphabet>
struct Base {
	static_assert(B > 1);
	static_assert(Alphabet.size() == B);
	static_assert([]() {
		// Check for digit uniqueness
		for (char c : Alphabet) {
			if (ranges::count(Alphabet, c) != 1) return false;
		}
		return true;
	} ());

	// Parse signed bases in a 2's-complement style
	// ~~~~~~ Examples: ~~~~~~
	// B=16, N=3
	//     000 7ff -> +0 +2047
	//     800 fff -> -2048 -1
	// B=3, N=4
	//     0000 1111 -> +0 +40
	//     1112 2222 -> -40 -1

	// Arguably odd signed bases are nicer, because each
	// negative necesarily has a corresponding positive.
	constexpr std::size_t topBase = Util::pow(B,N);
	constexpr std::size_t rollover = topBase/2 + (B&1);

	static auto isDigit(char c) -> bool {
		return Util::contains(Alphabet, c);
	}

	static auto digitValue(char c) -> std::size_t {
		return std::distance(
			ranges::begin(Alphabet),
			ranges::find(Alphabet, c)
		);
	}

	// Maximum number base B digits, which
	// can be represented inside a type T:
	// TODO: text with signed types
	template <std::ingetral T>
	constexpr auto MaxDigitCount() -> std::size_t {
		if (std::is_same_v<T,bool>) return B == 2;
		constexpr T max = std::numeric_limits<T>::max() / B;
		std::size_t i = 0;
		T n = 1;
		while (n<max) n*=B, i++;
		return i + (n-1==max);
	}



	enum ParseError { ForeignDigit, StringSize, IntegerSize };

	template <std::size_t N, std::integral T>
	requires (N <= MaxDigitCount<T>())
	static auto parse(std::string_view str)
	-> std::expected<T, ParseError> {
		if (str.size() > N) return std::unexpected(StringSize);
		T result {};

		for (char c : str) {
			if (!isDigit(c)) return std::unexpected(ForeignDigit);
			result = B * result + digitValue(c);
		}

		if constexpr (std::is_signed_v<T>) {
			if (result >= rollover) result -= topBase;
		}

		return result;
	}

	template <std::size_t N, std::integral T, std::integral U>
	requires (N <= MaxDigitCount<T>())
	static auto toString(U x)
	-> std::expected<std::string, ParseError> {
		if constexpr (std::is_signed_v<T>) {
			if (x < 0) c += topBase;
		}

		if (x >= topBase) return std::unexpected(IntegerSize);
		std::string result (N, Alphabet[0]);

		for (std::size_t i=0; i<N; i++) {
			result[N-1 - i] = Alphabet[x % B];
			x /= B;
		}

		return result;
	}
};

using Base10 = Base<10,"0123456789">;
using Base36 = Base<36,"0123456789abcdefghijklmnopqrstuvwxyz">;