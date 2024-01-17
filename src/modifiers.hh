#pragma once
#include "types.hh"
#include <array>
#include <vector>

class Affine {
	std::array<float,9> m;
public:
	Affine();
	Affine(std::array<float,9> m);
	std::vector<Atom> operator()(std::span<const Atom> atoms);
};

class Array {
	Affine transformation;
	std::size_t n;
public:
	std::vector<Atom> operator()(std::span<const Atom> atoms);
};

using Modifier = std::variant<Affine, Array/*, ... */>;