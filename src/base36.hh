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
	template <std::size_t N>
	static constexpr auto topBase() -> std::size_t {
		return Util::pow(B,N);
	}

	template <std::size_t N>
	static constexpr auto rollover() -> std::size_t {
		return topBase<N>()/2 + (B&1);
	}

	// Maximum number base B digits, which
	// can be represented inside a type T:
	// TODO: text with signed types
	template <std::integral T>
	static constexpr auto MaxDigitCount() -> std::size_t {
		if (std::is_same_v<T,bool>) return B == 2;
		constexpr T max = std::numeric_limits<T>::max() / B;
		std::size_t i = 0;
		T n = 1;
		while (n<max) n*=B, i++;
		return i + (n-1==max);
	}

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	static auto isDigit(char c) -> bool {
		return Util::contains(Alphabet, c);
	}

	static auto digitValue(char c) -> std::size_t {
		return std::distance(
			ranges::begin(Alphabet),
			ranges::find(Alphabet, c)
		);
	}

	enum ParseError { ForeignDigit, StringSize/*, IntegerSize*/ };

	/* ~~ Compile-time Size ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
			if (result >= rollover<N>()) result -= topBase<N>();
		}

		return result;
	}

	template <std::size_t N, std::integral T, std::integral U>
	requires (N <= MaxDigitCount<T>())
	static auto toString(U x)
	-> std::string {
		if constexpr (std::is_signed_v<T>) {
			if (x < 0) x += topBase<N>();
		}

		std::string result (N, Alphabet[0]);
		for (std::size_t i=0; i<N; i++) {
			result[N-1 - i] = Alphabet[x % B];
			x /= B;
		}

		return result;
	}

	/* ~~ Run-time Size ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	template <std::integral T>
	requires (std::is_unsigned_v<T>)
	static auto parse_n(std::string_view str)
	-> std::expected<T, ParseError> {
		if (str.size() > MaxDigitCount<T>()) {
			return std::unexpected(StringSize);
		}

		T result {};
		for (char c : str) {
			if (!isDigit(c)) return std::unexpected(ForeignDigit);
			result = B * result + digitValue(c); 
		}

		return result;
	}

	template <std::integral T>
	requires (std::is_unsigned_v<T>)
	static auto toString_n(T x)
	-> std::string {
		std::string result {};
		do {
			result = Alphabet[x % B] + result;
			x /= B;
		} while (x > 0);

		return result;
	}
};

using Base10 = Base<10,"0123456789">;
using Base36 = Base<36,"0123456789abcdefghijklmnopqrstuvwxyz">;