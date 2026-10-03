/**
 * @file file_splitter.cpp
 * @brief Реализация разбиения файла на диапазоны.
 */

#include "file_splitter.hpp"

#include <fstream>
#include <stdexcept>

namespace mtfind {

namespace {

/**
 * @brief Ищет первый '\n' начиная с позиции @p from.
 * @return Позиция символа после '\n' или @p file_size, если '\n' не найден.
 */
std::size_t seek_past_newline(std::ifstream& in,
                              std::size_t from,
                              std::size_t file_size) {
    if (from >= file_size) {
        return file_size;
    }
    in.clear();
    in.seekg(static_cast<std::streamoff>(from), std::ios::beg);

    char c = 0;
    std::size_t pos = from;
    while (pos < file_size && in.get(c)) {
        ++pos;
        if (c == '\n') {
            return pos;
        }
    }
    return file_size;
}

} // namespace

std::vector<Range> split_file(const std::string& path, std::size_t num_parts) {
    if (num_parts == 0) {
        throw std::runtime_error("num_parts must be > 0");
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + path);
    }

    in.seekg(0, std::ios::end);
    const auto end_pos = in.tellg();
    if (end_pos <= 0) {
        return {};
    }
    const std::size_t file_size = static_cast<std::size_t>(end_pos);

    if (file_size == 0) {
        return {};
    }

    // Базовый размер куска. Остаток раскидываем по первым диапазонам —
    // это даёт почти равномерную загрузку вне зависимости от длины строк.
    const std::size_t base_chunk = file_size / num_parts;

    std::vector<Range> ranges;
    ranges.reserve(num_parts);

    std::size_t start = 0;
    for (std::size_t i = 1; i < num_parts; ++i) {
        // Целевая граница куска. Для последнего сегмента она всё равно
        // будет отодвинута к '\n', а конец выставим в file_size.
        std::size_t target = base_chunk * i;
        if (target >= file_size) {
            break;
        }

        // Сдвигаем целевую границу к ближайшему '\n' вправо (за него).
        std::size_t next_start = seek_past_newline(in, target, file_size);
        if (next_start >= file_size) {
            // Упёрлись в конец: дальше резать нечего.
            break;
        }
        if (next_start <= start) {
            // Выродились: '\n' слишком далеко или граница совпала. Пропускаем.
            continue;
        }

        ranges.push_back(Range{start, next_start});
        start = next_start;
    }

    // Последний диапазон идёт до конца файла.
    if (start < file_size) {
        ranges.push_back(Range{start, file_size});
    }

    return ranges;
}

} // namespace mtfind