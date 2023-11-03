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

using Fn_Arg = std::span<const Atom>;
using Fn_Ret = std::vector<Atom>;

template <typename... Ts>
concept ModCallable = requires {
	( (std::is_invocable_r_v<Fn_Ret, Ts, Fn_Arg>) && ... );
};

static_assert(ModCallable<Array, Affine>);