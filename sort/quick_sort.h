#pragma once

#include <cstddef>
#include <cstdint>

namespace quick_sort {

void sort_seq(std::int32_t* data, std::size_t size, std::uint64_t seed = 466);

void sort_par(std::int32_t* data, std::size_t size, std::uint64_t seed = 466);

namespace detail {

void sort_seq_impl(std::int32_t* data, std::size_t size, std::uint64_t rng_state);

}

}
