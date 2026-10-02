/**
 * @file searcher.cpp
 * @brief Реализация поиска вхождений маски.
 */

#include "searcher.hpp"

#include <stdexcept>

namespace mtfind {

Searcher::Searcher(std::string mask) : mask_(std::move(mask)) {
    if (mask_.empty()) {
        throw std::invalid_argument("mask must not be empty");
    }
    build_shift_table();
}

void Searcher::build_shift_table() {
    const std::size_t m = mask_.size();

    // По умолчанию сдвиг равен длине маски: если последний символ окна
    // не встречается в маске, окно можно сдвинуть на всю длину.
    shift_.fill(m);

    // Для каждого символа маски (кроме последнего) сдвиг равен расстоянию
    // от его последнего вхождения до конца маски. Это вариант Horspool:
    // сравнение идёт с последним символом окна, а не с первым несовпавшим.
    for (std::size_t i = 0; i + 1 < m; ++i) {
        const auto c = static_cast<unsigned char>(mask_[i]);
        shift_[c] = m - 1 - i;
    }
}

std::vector<Match> Searcher::find_all(const std::string& line) const {
    std::vector<Match> result;

    const std::size_t n = line.size();
    const std::size_t m = mask_.size();

    // Если маска длиннее строки — вхождений быть не может.
    if (m > n) {
        return result;
    }

    const std::size_t last = m - 1;
    std::size_t pos = 0;

    // Основной цикл BMH. pos — позиция начала окна в строке.
    while (pos + m <= n) {
        // Сравнение справа налево. j — индекс в маске.
        // Используем std::size_t и "магическое" значение SIZE_MAX как -1:
        // как только j переполнится после --j, условие j < m перестанет
        // выполняться и цикл завершится — это означает полное совпадение.
        std::size_t j = last;
        while (j < m && line[pos + j] == mask_[j]) {
            --j;
        }

        if (j >= m) {
            // Полное совпадение: j "переполнилось" ниже нуля.
            result.push_back(Match{pos, mask_});
            // Вхождения не должны пересекаться — продолжаем поиск
            // сразу после конца найденного вхождения.
            pos += m;
        } else {
            // Несовпадение: сдвигаем окно по таблице Хорспула,
            // используя последний символ текущего окна.
            const auto c = static_cast<unsigned char>(line[pos + last]);
            pos += shift_[c];
        }
    }

    return result;
}

} // namespace mtfind