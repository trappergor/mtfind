/**
 * @file mask.cpp
 * @brief Реализация компиляции маски.
 */

#include "mask.hpp"

#include <stdexcept>

namespace mtfind {

CompiledMask compile_mask(const std::string& mask) {
    if (mask.empty()) {
        throw std::invalid_argument("mask must not be empty");
    }

    CompiledMask c;
    c.mask = mask;
    c.length = mask.size();

    // Проходим по маске и выделяем максимальные последовательности
    // символов, не равных '?'. Каждая такая последовательность — сегмент.
    std::size_t i = 0;
    while (i < mask.size()) {
        if (mask[i] == '?') {
            ++i;
            continue;
        }
        const std::size_t start = i;
        while (i < mask.size() && mask[i] != '?') {
            ++i;
        }
        c.segments.push_back(Segment{start, mask.substr(start, i - start)});
    }

    // Якорь — самый длинный сегмент. При равенстве длин выбираем первый:
    // это детерминированно и удобно для тестов.
    std::size_t best_len = 0;
    for (std::size_t k = 0; k < c.segments.size(); ++k) {
        if (c.segments[k].text.size() > best_len) {
            best_len = c.segments[k].text.size();
            c.anchor_index = k;
        }
    }

    return c;
}

} // namespace mtfind