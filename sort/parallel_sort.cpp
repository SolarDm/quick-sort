#include "quick_sort.h"
#include "random.h"

#include <memory>

#include <parlay/parallel.h>

namespace quick_sort::detail {

const std::size_t BLOCK_SIZE = 1024;
const std::size_t SEQ_THRESHOLD = 16384;
const std::size_t SCAN_THRESHOLD = 1024;

struct BlockInfo {
    std::size_t less;
    std::size_t equal;

    BlockInfo& operator+=(const BlockInfo& other) {
        less += other.less;
        equal += other.equal;
        return *this;
    }
};

std::int32_t choose_pivot(const std::int32_t* data, std::size_t size, std::uint64_t& rnd_state) {
    const std::int32_t a = data[splitmix64(rnd_state) % size];
    const std::int32_t b = data[splitmix64(rnd_state) % size];
    const std::int32_t c = data[splitmix64(rnd_state) % size];

    if (a < b) {
        if (b < c) return b;
        return a < c ? c : a;
    }
    if (a < c) return a;
    return b < c ? c : b;
}

void process_block(const std::int32_t* data, std::int32_t pivot, std::size_t size,
    BlockInfo* info) {
    std::size_t less = 0;
    std::size_t equal = 0;

    for (std::size_t i = 0; i < size; i++) {
        less += data[i] < pivot;
        equal += data[i] == pivot;
    }

    info->less = less;
    info->equal = equal;
}

void count_blocks(const std::int32_t* data, std::size_t size, std::int32_t pivot, BlockInfo* blocks) {
    if (size == 0) {
        return;
    }

    if (size <= BLOCK_SIZE) {
        process_block(data, pivot, size, &blocks[0]);
        return;
    }

    std::size_t blocks_count = size / BLOCK_SIZE + (size % BLOCK_SIZE > 0);
    std::size_t m = BLOCK_SIZE * (blocks_count / 2);
    parlay::par_do(
        [&]{ count_blocks(data, m, pivot, blocks); },
        [&]{ count_blocks(data + m, size - m, pivot, blocks + blocks_count / 2); }
    );
}

void down(BlockInfo* blocks, std::size_t l, std::size_t r, BlockInfo left_sum) {
    if (r - l == 1) {
        blocks[l] = left_sum;
        return;
    }

    std::size_t m = l + (r - l) / 2;
    BlockInfo left = blocks[m - 1];

    const auto left_action = [&]{ down(blocks, l, m, left_sum); };
    const auto right_action = [&]{ 
        BlockInfo new_left = left_sum;
        new_left += left;
        down(blocks, m, r, new_left);
    };

    if (r - l > SCAN_THRESHOLD) {
        parlay::par_do(left_action, right_action);
    } else {
        left_action();
        right_action();
    }
}

void up(BlockInfo* blocks, std::size_t l, std::size_t r) {
    if (r - l == 1) {
        return;
    }

    std::size_t m = l + (r - l) / 2;

    const auto left_action = [&]{ up(blocks, l, m); };
    const auto right_action = [&]{ up(blocks, m, r); };

    if (r - l > SCAN_THRESHOLD) {
        parlay::par_do(left_action, right_action);
    } else {
        left_action();
        right_action();
    }

    blocks[r - 1] += blocks[m - 1];
}

BlockInfo exclusive_scan(BlockInfo* blocks, std::size_t count) {
    if (count == 0) {
        return {0, 0};
    }

    up(blocks, 0, count);
    BlockInfo result = blocks[count - 1];

    down(blocks, 0, count, {0, 0});

    return result;
}

void fill_block(const std::int32_t* data, std::int32_t* buffer, std::size_t size,
    std::int32_t pivot, BlockInfo offset, BlockInfo total, std::size_t skip_count) {
    std::size_t less_ind = offset.less;
    std::size_t equal_ind = total.less + offset.equal;
    std::size_t greater_ind = total.less + total.equal + (skip_count - offset.less - offset.equal);

    for (std::size_t i = 0; i < size; i++) {
        if (data[i] < pivot) {
            buffer[less_ind++] = data[i];
        } else if (data[i] == pivot) {
            buffer[equal_ind++] = data[i];
        } else {
            buffer[greater_ind++] = data[i];
        }
    }
}

void fill_blocks(const std::int32_t* data, std::int32_t* buffer, std::size_t size,
    std::int32_t pivot, const BlockInfo* offsets, BlockInfo total, std::size_t skip_count) {
    if (size == 0) {
        return;
    }

    if (size <= BLOCK_SIZE) {
        fill_block(data, buffer, size, pivot, offsets[0], total, skip_count);
        return;
    }


    std::size_t blocks_count = size / BLOCK_SIZE + (size % BLOCK_SIZE > 0);
    std::size_t m_blocks = blocks_count / 2;
    std::size_t m = BLOCK_SIZE * m_blocks;
    parlay::par_do(
        [&]{ fill_blocks(data, buffer, m, pivot, offsets, total, skip_count); },
        [&]{ fill_blocks(data + m, buffer, size - m, pivot, offsets + m_blocks, total, skip_count + m); }
    );
}

void copy_data(std::int32_t* data, std::int32_t* buffer, std::size_t size) {
    if (size < BLOCK_SIZE) {
        for (std::size_t i = 0; i < size; i++) {
            data[i] = buffer[i];
        }
        return;
    }

    std::size_t m = size / 2;
    parlay::par_do(
        [&]{ copy_data(data, buffer, m); },
        [&]{ copy_data(data + m, buffer + m, size - m); }
    );
}

BlockInfo partition_par(std::int32_t* data, std::int32_t* buffer, std::size_t size,
    std::int32_t pivot, BlockInfo* blocks) {
    count_blocks(data, size, pivot, blocks);

    std::size_t blocks_count = size / BLOCK_SIZE + ((size % BLOCK_SIZE) > 0);
    BlockInfo total = exclusive_scan(blocks, blocks_count);

    fill_blocks(data, buffer, size, pivot, blocks, total, 0);

    return total;
}

void sort_par_impl(std::int32_t* data, std::int32_t* buffer, std::size_t size,
                         BlockInfo* blocks, std::uint64_t rnd_state, bool result_in_buffer) {
    if (size <= SEQ_THRESHOLD) {
        sort_seq_impl(data, size, rnd_state);
        if (result_in_buffer) {
            copy_data(buffer, data, size);
        }
        return;
    }

    std::int32_t pivot = choose_pivot(data, size, rnd_state);

    BlockInfo total = partition_par(data, buffer, size, pivot, blocks);

    if (!result_in_buffer) {
        copy_data(data + total.less, buffer + total.less, total.equal);
    }

    std::size_t greater_offset = total.less + total.equal;
    // Чтобы блоков хватило на всех уровнях (учитывая то, что при разрезании одного блока нам становится нужно 2)
    std::size_t left_blocks = 2 * (total.less / BLOCK_SIZE);
    parlay::par_do(
        [&]{ sort_par_impl(buffer, data, total.less, blocks, rnd_state, !result_in_buffer); },
        [&]{ sort_par_impl(buffer + greater_offset, data + greater_offset, size - greater_offset,
                    blocks + left_blocks, rnd_state + 1, !result_in_buffer); }
    );
}

}

namespace quick_sort {

void sort_par(std::int32_t* data, std::size_t size, std::uint64_t seed) {
    if (size <= detail::SEQ_THRESHOLD) {
        detail::sort_seq_impl(data, size, seed);
        return;
    }

    std::unique_ptr<std::int32_t[]> buffer(new std::int32_t[size]);

    // Размер такой, чтобы хватило, учитывая все разрезы
    std::unique_ptr<detail::BlockInfo[]> blocks(
        new detail::BlockInfo[2 * (size / detail::BLOCK_SIZE)]);
    detail::sort_par_impl(data, buffer.get(), size, blocks.get(), seed, false);
}

}
