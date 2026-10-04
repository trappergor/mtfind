/**
 * @file searcher.cpp
 * @brief Реализация поиска вхождений маски (включая '?').
 */

#include "searcher.hpp"

#include <stdexcept>

namespace mtfind {

// ----------------------------- BmhMatcher ---------------------------------

BmhMatcher::BmhMatcher(std::string needle) : needle_(std::move(needle)) {
    const std::size_t m = needle_.size();
    // Сдвиг по умолчанию. Для пустой иглы используем 1, чтобы не зациклиться,
    // хотя search() пустую иглу всё равно не ищет.
    shift_.fill(m == 0 ? 1 : m);

    // Таблица Хорспула: сдвиг при несовпадении последнего символа
    // окна равен расстоянию от его последнего вхождения в игле
    // (кроме последнего символа) до конца иглы.
    for (std::size_t i = 0; i + 1 < m; ++i) {
        const auto c = static_cast<unsigned char>(needle_[i]);
        shift_[c] = m - 1 - i;
    }
}

std::size_t BmhMatcher::search(const std::string& haystack,
                               std::size_t from) const noexcept {
    const std::size_t n = haystack.size();
    const std::size_t m = needle_.size();

    if (m == 0 || m > n || from + m > n) {
        return npos;
    }

    const std::size_t last = m - 1;
    std::size_t pos = from;

    // Сравнение справа налево. 
    // j == SIZE_MAX после --j означает полное совпадение.
    while (pos + m <= n) {
        std::size_t j = last;
        while (j < m && haystack[pos + j] == needle_[j]) {
            --j;
        }
        if (j >= m) {
            return pos;
        }
        pos += shift_[static_cast<unsigned char>(haystack[pos + last])];
    }
    return npos;
}

// ------------------------------ Searcher ----------------------------------

Searcher::Searcher(std::string mask) : compiled_(compile_mask(mask)) {
    if (compiled_.anchor_index != no_anchor) {
        anchor_matcher_ =
            BmhMatcher(compiled_.segments[compiled_.anchor_index].text);
    }
}

std::vector<Match> Searcher::find_all(const std::string& line) const {
    const std::size_t n = line.size();
    const std::size_t m = compiled_.length;

    if (m == 0 || m > n) {
        return {};
    }

    // маска состоит только из '?'.
    if (compiled_.segments.empty()) {
        return find_all_wildcards_only(line);
    }

    // маска без '?' — единственный сегмент, совпадающий с маской.
    if (compiled_.segments.size() == 1
        && compiled_.segments[0].offset == 0
        && compiled_.segments[0].text.size() == m) {
        return find_all_exact(line);
    }

    // смешанная маска.
    return find_all_with_anchor(line);
}

std::vector<Match> Searcher::find_all_wildcards_only(const std::string& line) const {
    std::vector<Match> result;
    const std::size_t m = compiled_.length;
    const std::size_t n = line.size();

    // Каждое вхождение — блок из m подряд идущих символов. Чтобы вхождения
    // не пересекались, шагаем ровно на m.
    for (std::size_t pos = 0; pos + m <= n; pos += m) {
        result.push_back(Match{pos, line.substr(pos, m)});
    }
    return result;
}

std::vector<Match> Searcher::find_all_exact(const std::string& line) const {
    std::vector<Match> result;
    const std::size_t m = compiled_.length;
    const std::size_t n = line.size();

    std::size_t from = 0;
    while (from + m <= n) {
        const std::size_t p = anchor_matcher_.search(line, from);
        if (p == npos) {
            break;
        }
        // Точное совпадение: текст вхождения совпадает с маской.
        result.push_back(Match{p, compiled_.mask});
        // Непересекаемость: следующий старт не раньше конца найденного.
        from = p + m;
    }
    return result;
}

std::vector<Match> Searcher::find_all_with_anchor(const std::string& line) const {
    std::vector<Match> result;
    const std::size_t n = line.size();
    const std::size_t m = compiled_.length;

    const Segment& anchor = compiled_.segments[compiled_.anchor_index];
    const std::size_t anchor_len = anchor.text.size();
    const std::size_t anchor_off = anchor.offset;

    // Начало вхождения не может превышать n - m: иначе оно не влезет.
    const std::size_t max_candidate = n - m;

    std::size_t next_allowed = 0; // минимально допустимое начало кандидата
    std::size_t from = 0;         // откуда искать очередной якорь

    while (from + anchor_len <= n) {
        const std::size_t p = anchor_matcher_.search(line, from);
        if (p == npos) {
            break;
        }

        // Якорь не может начинаться раньше, чем его смещение в маске:
        // иначе начало вхождения ушло бы в отрицательную область.
        if (p < anchor_off) {
            from = anchor_off;
            continue;
        }

        const std::size_t candidate = p - anchor_off;

        // Начало вхождения ушло правее допустимого: дальнейшие якоря
        // только увеличивают candidate, поэтому дальше искать бессмысленно.
        if (candidate > max_candidate) {
            break;
        }

        // Вхождение пересеклось бы с уже принятым: перескакиваем сразу
        // к ближайшему якорю, дающему candidate >= next_allowed.
        if (candidate < next_allowed) {
            from = next_allowed + anchor_off;
            continue;
        }

        if (verify_at(line, candidate)) {
            result.push_back(Match{candidate, line.substr(candidate, m)});
            next_allowed = candidate + m;
            from = next_allowed + anchor_off;
        } else {
            // Кандидат не подошёл: пробуем следующий якорь.
            from = p + 1;
        }
    }

    return result;
}

bool Searcher::verify_at(const std::string& line,
                         std::size_t candidate) const noexcept {
    // candidate + m <= line.size() гарантировано вызывающим кодом.
    // Якорный сегмент пропускаем: BMH его уже подтвердил в этой позиции.
    for (std::size_t k = 0; k < compiled_.segments.size(); ++k) {
        if (k == compiled_.anchor_index) {
            continue;
        }
        const auto& seg = compiled_.segments[k];
        const std::size_t at = candidate + seg.offset;
        for (std::size_t i = 0; i < seg.text.size(); ++i) {
            if (line[at + i] != seg.text[i]) {
                return false;
            }
        }
    }
    return true;
}

} // namespace mtfind