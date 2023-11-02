class Affine {
	std::array<float,9> m;
public:
	Affine() : m{1,0,0 , 0,1,0 , 0,0,1} {}
	Affine(std::array<float,9> m) : m{m} {}

	std::vector<Atom> operator()(std::span<const Atom> atoms) {
		std::vector<Atom> result {};
		/* ... */
		return result;
	}
};

class Array {
	Affine transformation;
	std::size_t n;
public:
	std::vector<Atom> operator()(std::span<const Atom> atoms) {
		std::vector<Atom> result {};
		/* ... */
		return result;
	}
};