#include "quick_sort.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using Sort = void (*)(std::int32_t*, std::size_t, std::uint64_t);

void check(Sort sort, const std::vector<std::int32_t>& input) {
    auto expected = input;
    std::sort(expected.begin(), expected.end());

    for (std::uint64_t seed : {0, 466, 7584984}) {
        constexpr std::int32_t guard = 123456789;
        std::vector<std::int32_t> actual(input.size() + 2, guard);
        std::copy(input.begin(), input.end(), actual.begin() + 1);
        sort(actual.data() + 1, input.size(), seed);
        if (actual.front() != guard || actual.back() != guard ||
            !std::equal(expected.begin(), expected.end(), actual.begin() + 1)) {
            throw std::runtime_error("size=" + std::to_string(input.size())
                                     + ", seed=" + std::to_string(seed));
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2 || (std::string(argv[1]) != "seq" && std::string(argv[1]) != "par")) {
        std::cerr << "Usage: correctness_tests seq|par\n";
        return 1;
    }
    const bool parallel = std::string(argv[1]) == "par";
    const Sort sort = parallel ? quick_sort::sort_par : quick_sort::sort_seq;

    try {
        sort(nullptr, 0, 466);
        sort({}, 0, 466);

        std::mt19937 rnd(42);
        std::uniform_int_distribution<std::int32_t> random_value(
            std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max());
    
        check(sort, {random_value.min(), 0, random_value.max(), -1, random_value.min()});
        for (std::size_t n : {1, 2, 8191, 8192, 8193, 65535, 65536, 65537, 73729, 2097151}) {
            std::vector<std::int32_t> input(n);
            for (auto& value : input) value = random_value(rnd);
            check(sort, input);
            std::sort(input.begin(), input.end());
            check(sort, input);
            std::reverse(input.begin(), input.end());
            check(sort, input);
            for (auto& value : input) value = static_cast<std::int32_t>(rnd() % 5) - 2;
            check(sort, input);
            std::fill(input.begin(), input.end(), 7);
            check(sort, input);
        }

        std::cout << "OK: " << argv[1] << '\n';
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
