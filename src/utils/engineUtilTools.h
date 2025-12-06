#pragma once
#include <cstdint>
#include <vector>
#include <glm/gtx/hash.hpp>

#include <functional>

namespace lve {

	template <typename T, typename... Rest>
	void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		if constexpr (sizeof...(rest) > 0)
			hashCombine(seed, rest...);
	}

} // namespace lve

/*
namespace eut {
	struct Handle {
		int32_t index = -1;
		int32_t generation = -1;

		bool operator==(const Handle &v) const {
			return (index == v.index && generation == v.generation);
		}
	};

	template <typename T>
	class HVector {
	public:
		HVector(size_t size) { vector.resize(size); }
		HVector() = default;

		HVector(const HVector &o)
		{
			vector = o.vector;
			aliveList = o.deadList;
			generations = o.generations;
			freeList = o.freeList;
		}
		HVector &operator=(const HVector &o) {
			vector = o.vector;
			aliveList = o.deadList;
			generations = o.generations;
			freeList = o.freeList;
			return *this;
		}

		template<typename U>
		Handle push_back(U&& o)
		{
			uint32_t index;

			if (!freeList.empty())
			{
				index = freeList.back();
				freeList.pop_back();

				generations[index]++;
				vector[index] = std::forward<U>(o);
				aliveList[index] = true;
			}
			else
			{
				index = static_cast<uint32_t>(vector.size());
				vector.push_back(std::forward<U>(o));
				generations.push_back(0);
				aliveList.push_back(true);
			}

			return Handle{ index, generations[index] };
		}


		void erase(Handle h)
		{
			if (!aliveList[h.index]) return;
			aliveList[h.index] = false;
			generations[h.index]++;
			freeList.push_back(h.index);
		}

		std::vector<T> vector;
	private:
		std::vector<bool> aliveList;
		std::vector<uint32_t> generations; // stores how many times that slot has been reused
		std::vector<uint32_t> freeList; // holds indices of deleted slots that can be reused
	};
}*/

