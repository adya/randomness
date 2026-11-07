#pragma once

namespace randomness
{
	/// A generic picker that can draw an option from a set.
	template <class P, class Rng>
	concept picker = std::uniform_random_bit_generator<Rng> && requires(P& picker, Rng& rng) {
		typename P::value_type;
		{ picker.draw(rng) } -> std::convertible_to<typename P::value_type>;
	};
}
