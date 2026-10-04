/**
 * @file worker.cpp
 * @brief Реализация обработки одного диапазона.
 */

#include "worker.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace mtfind {

namespace {

/// Убирает завершающий '\r' (для файлов с CRLF).
void strip_cr(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

} // namespace

WorkerResult run_worker(const std::string& filename,
                        const Range& range,
                        const Searcher& searcher) {
    WorkerResult result;

    // Пустой диапазон: файл создаём, но ничего не читаем.
    if (range.end <= range.start) {
        return result;
    }

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + filename);
    }
    in.seekg(static_cast<std::streamoff>(range.start), std::ios::beg);

    std::size_t relative_line = 0;
    std::string line;

    // Читаем строки, пока не исчерпали диапазон. 
    // split_file гарантирует, что каждая строка целиком лежит в [start, end),
    // поэтому цикл не перескочит за границу диапазона.
    while (static_cast<std::size_t>(in.tellg()) < range.end) {
        if (!std::getline(in, line)) {
            break;
        }
        ++relative_line;
        strip_cr(line);

        for (const auto& m : searcher.find_all(line)) {
            // Копируем текст как есть: если вхождение начинается с пробела,
            // он сохраняется.
            result.matches.push_back(
                MatchRecord{relative_line, m.position, m.text});
        }
    }

    result.line_count = relative_line;
    return result;
}

} // namespace mtfind