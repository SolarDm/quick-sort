#include "quick_sort.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <random>

int main(int argc, char** argv) {
    try {
        if (argc != 2 || (std::string(argv[1]) != "seq" && std::string(argv[1]) != "par")) {
            throw std::runtime_error("Usage: PARLAY_NUM_THREADS=N benchmark seq|par");
        }

        const std::string mode = argv[1];
        constexpr std::size_t size = 100000000;

        std::mt19937 rnd(887);
        std::unique_ptr<std::int32_t[]> data(new std::int32_t[size]);
        for (std::size_t i = 0; i < size; ++i) {
            data[i] = static_cast<std::int32_t>(static_cast<std::int64_t>(rnd()) - 2147483648LL);
        }

        const auto start = std::chrono::steady_clock::now();
        if (mode == "seq") {
            quick_sort::sort_seq(data.get(), size);
        } else { 
            quick_sort::sort_par(data.get(), size);
        }
        const auto finish = std::chrono::steady_clock::now();

        const double seconds = std::chrono::duration<double>(finish - start).count();

        for (std::size_t i = 0; i < size; ++i) {
            if (i > 0 && data[i - 1] > data[i]) {
                throw std::runtime_error("Result is not sorted");
            }
        }

        std::cout << std::setprecision(9) << seconds << '\n';
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
