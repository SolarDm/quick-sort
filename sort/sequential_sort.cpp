#include "quick_sort.h"
#include "random.h"

namespace quick_sort::detail {

template <typename Predicate>
std::size_t partition_seq(std::int32_t* data, std::size_t size, Predicate is_l) {
    std::size_t l = 0;
    std::size_t r = size;

    while (l < r) {
        while (l < r && is_l(data[l])) {
            l++;
        }
        while (l < r && !is_l(data[r - 1])) {
            r--;
        }
        if (l < r) {
            const std::int32_t value = data[l];
            data[l] = data[r - 1];
            data[r - 1] = value;
            l++;
            r--;
        }
    }

    return l;
}

void sort_seq_impl(std::int32_t* data, std::size_t size, std::uint64_t rng_state) {
    if (size <= 1) {
        return;
    }

    const std::int32_t pivot = data[detail::splitmix64(rng_state) % size];

    const std::size_t equal_end = partition_seq(
        data, size, [pivot](std::int32_t value) { return value <= pivot; });
    const std::size_t less_end = partition_seq(
        data, equal_end, [pivot](std::int32_t value) { return value < pivot; });

    sort_seq_impl(data, less_end, rng_state);
    sort_seq_impl(data + equal_end, size - equal_end, rng_state + 1);
}

}

namespace quick_sort {

void sort_seq(std::int32_t* data, std::size_t size, std::uint64_t seed) {
    detail::sort_seq_impl(data, size, seed);
}

}
