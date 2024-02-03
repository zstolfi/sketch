#pragma once
#include "util.hh"
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <concepts>

// Header file for ANY base (but 36 and 10 mainly)

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

	/* ~~ Metafunctions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
	static constexpr auto capacity() -> std::size_t {
		return Util::pow(B,N);
	}

	template <std::size_t N>
	static constexpr auto rollover() -> std::size_t {
		return capacity<N>()/2 + (B&1);
	}

	// Maximum number base B digits, which
	// can be represented inside a type T:
	template <std::integral T>
	static constexpr auto MaxDigitCount() -> std::size_t {
		using U = std::make_unsigned_t<T>;
		constexpr U max = std::numeric_limits<U>::max() / B;
		std::size_t i = 0;
		U n = 1;
		while (n<max) n*=B, i++;
		return i + (n-1==max);
	}

	template <>
	static constexpr auto MaxDigitCount<bool>() -> std::size_t {
		return B == 2;
	}

	/* ~~ Member Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	static auto isDigit(char c) -> bool {
		return Util::contains(Alphabet, c);
	}

	static auto digitValue(char c) -> std::size_t {
		return std::distance(
			ranges::begin(Alphabet),
			ranges::find(Alphabet, c)
		);
	}

	// Compiletime Size:

	template <std::size_t N, std::integral T>
	requires (N <= MaxDigitCount<T>())
	static auto parse(std::string_view str) -> std::optional<T> {
		if (str.size() > N) return std::nullopt;

		T result {};
		for (char c : str) {
			if (!isDigit(c)) return std::nullopt;
			result = B * result + digitValue(c);
		}

		if constexpr (std::is_signed_v<T>) {
			if (result >= rollover<N>()) result -= capacity<N>();
		}

		return result;
	}

	template <std::size_t N, std::integral T, std::integral U>
	requires (N <= MaxDigitCount<T>())
	static auto toString(U x)
	-> std::string {
		if constexpr (std::is_signed_v<T>) {
			if (x < 0) x += capacity<N>();
		}

		std::string result (N, Alphabet[0]);
		for (std::size_t i=0; i<N; i++) {
			result[N-1 - i] = Alphabet[x % B];
			x /= B;
		}

		return result;
	}

	// Runtime Size:

	template <std::integral T>
	requires (std::is_unsigned_v<T>)
	static auto parse_n(std::string_view str) -> std::optional<T> {
		if (str.size() > MaxDigitCount<T>()) {
			return std::nullopt;
		}

		T result {};
		for (char c : str) {
			if (!isDigit(c)) return std::nullopt;
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

	/* ~~ Parse N Tuples ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	template <std::size_t N, std::integral T>
	struct Number_t { static constexpr auto size = N; using type = T; };

	template <typename... Nums, typename... Args, typename Res>
	static auto parseTuples(
		std::string_view str,
		std::tuple<Nums...> nums,
		Res (*makeObj)(Args...)
	) -> std::optional<std::vector<Res>> {
		static_assert(sizeof...(Nums) == sizeof...(Args));

		auto parseI = [&str]<std::size_t N, std::integral T>(
			std::size_t& i, Number_t<N,T>
		) {
			auto result = parse<N,T>(str.substr(i, N));
			i += N; return result;
		};

		constexpr std::size_t total = std::apply([](auto&&... num) {
			return ( num.size + ... );
		}, nums);

		std::vector<Res> result {};

		if (str.size()%total != 0) return std::nullopt;
		for (std::size_t i=0; i<str.size(); i+=total) {
			std::tuple<std::optional<Args>...> parsed {};

			[&]<std::size_t... I>(std::index_sequence<I...>) {
				(
					(std::get<I>(parsed) = parseI(i, std::get<I>(nums)))
					, ...
				);
			} (std::make_index_sequence<sizeof...(Nums)> {});

			bool allValid = true;
			std::apply([&](auto&&... res) {
				if (!( res && ... )) allValid = false;
				else result.push_back(makeObj(*res ...));
			}, parsed);

			if (!allValid) return std::nullopt;
		}

		return result;
	}
};

using Base10 = Base<10,"0123456789">;
using Base36 = Base<36,"0123456789abcdefghijklmnopqrstuvwxyz">;