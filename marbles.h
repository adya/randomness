#pragma once

namespace randomness::marbles
{
	template <class T>
	class marble;

	/// Marble Bag provides a controlled random draws from a predefined set of options.
	/// It emulates a process of drawing a finite number of marbles from a bag, each draw is random, but the expected outcome is preserved in short spans.
	///
	/// A marble represents a single item that can be drawn from the bag. Note that it can be either a value or a nested bag.
	/// Whenever we draw a marble that is a nested bag, we recurse into it, until we reach a value. This allows for a flexible hierarchical structure of bags and marbles.
	///
	/// You draw from a configured bag until it runs out of marbles. At that point, the next draw will refill the bag with the same proportions.
	/// To configure a bag, you "put" a specific number of marbles into it. 
	/// Once a bag is constructed with N marbles, it guarantees that across N draws you'll see the same number of each marble, but in a random order.
	///
	///     randomness::marbles::bag<Loot> bag{
	///         { 8, Loot::kCommon },                                 // 8 marbles
	///         { 2, { { 3, Loot::kRing }, { 1, Loot::kAmulet } } },  // 2 bags, containing 4 jewelry marbles
	///         { 1, rareBag },                                       // 1 bag, containing rare items
	///     };
	///
	/// In the example above, you're guaranteed to get 1 rare item every 11 draws. After 11 draws, the bag is refilled. 
	/// Note, that nested bag is not refilled, unless refill(recursive=true) is called.
	template <class T>
	class bag
	{
	public:
		using value_type = T;

		bag() = default;

		bag(std::initializer_list<marble<T>> items) :
			bag(std::vector<marble<T>>(items)) {}

		explicit bag(std::vector<marble<T>> items) :
			marbles(std::move(items)),
			remaining(marbles.size()) {
			refill();
		}

		/// The number of marble kinds in this bag, not counting the ones inside nested bags.
		std::size_t size() const {
			return static_cast<std::size_t>(capacity);
		}

		/// Takes the next marble out.
		template <class Rng>
		const T& draw(Rng& rng) {
			assert(capacity != 0 && "drawing from an empty bag");
			if (total == 0) {
				refill();  // the pass is used up
			}
			auto        slot = std::uniform_int_distribution<std::uint64_t>(0, total - 1)(rng);
			std::size_t i    = 0;
			while (slot >= remaining[i]) {
				slot -= remaining[i++];
			}
			--remaining[i];
			--total;
			return marbles[i].draw(rng);
		}

		/// Puts all the marbles back.
		/// Note, that nested bags keep their own pass unless `recursive` is set, which refills them too.
		void refill(bool recursive = false) {
			total = 0;
			for (std::size_t i = 0; i < marbles.size(); ++i) {
				total += (remaining[i] = marbles[i].count);
				if (recursive) {
					marbles[i].refill(recursive);
				}
			}
			capacity = total;
		}

	private:
		/// Content of the bag.
		std::vector<marble<T>> 	   marbles;
		/// Remaining number of each kind of marble in the bag.
		/// This vector is aligned with `marbles`.
		std::vector<std::uint64_t> remaining;  
		/// Total number of remaining marbles.
		std::uint64_t              total{ 0 };
		/// Number of marble kinds in the bag.
		std::uint64_t              capacity{ 0 };
	};

	/// A holder of a single value or a nested bag. A single thing that goes into a bag.
	template <class T>
	class marble
	{
	public:
		marble(std::size_t count, T value) :
			count(count), content(std::in_place_type<T>, std::move(value)) {}

		marble(std::size_t count, std::initializer_list<marble> marbles) :
			marble(count, bag<T>(marbles)) {}

		marble(std::size_t count, std::vector<marble<T>> marbles) :
			marble(count, bag<T>(std::move(marbles))) {}

		marble(std::size_t count, bag<T> inner) :
			count(count), content(std::in_place_type<bag<T>>, std::move(inner)) {}

	private:
		friend class bag<T>;

		/// Refills the bag inside.
		void refill(bool recursive) {
			if (auto* inner = std::get_if<bag<T>>(&content)) {
				inner->refill(recursive);
			}
		}

		template <class Rng>
		const T& draw(Rng& rng) {
			if (auto* inner = std::get_if<bag<T>>(&content)) {
				return inner->draw(rng);
			}
			return std::get<T>(content);
		}

		/// Number of marbles of this kind that were put in a bag
		std::size_t              count;    
		/// Content of this marble.
		std::variant<T, bag<T>>  content;  
	};
}
