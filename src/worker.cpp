/**
 * @file worker.cpp
 * @brief Реализация обработки одного диапазона.
 */

#include "worker.hpp"

#include <atomic>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <string>

namespace mtfind {

namespace {

/// Убирает завершающий '\r' (для файлов с CRLF).
void strip_cr(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

/**
 * @brief Генерирует уникальное имя временного файла в заданной директории.
 */
std::filesystem::path make_temp_path(const std::filesystem::path& dir) {
    static std::atomic<std::size_t> counter{0};
    const auto id = counter.fetch_add(1, std::memory_order_relaxed);
    return dir / ("mtfind_part_" + std::to_string(::getpid()) + "_"
                  + std::to_string(id) + ".tmp");
}

} // namespace

WorkerResult run_worker(const std::string& filename,
                        const Range& range,
                        const Searcher& searcher,
                        const std::filesystem::path& temp_dir) {
    WorkerResult result;
    result.temp_path = make_temp_path(temp_dir);

    std::ofstream out(result.temp_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot create temp file: "
                                 + result.temp_path.string());
    }

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

    // Читаем строку только если её начало ещё внутри диапазона.
    // split_file гарантирует, что вся строка целиком лежит в [start, end),
    // поэтому после чтения мы не выйдем за границу диапазона.
    while (static_cast<std::size_t>(in.tellg()) < range.end) {
        if (!std::getline(in, line)) {
            break;
        }
        ++relative_line;
        strip_cr(line);

        for (const auto& m : searcher.find_all(line)) {
            out << relative_line << ' ' << m.position << ' ' << m.text << '\n';
            ++result.match_count;
        }
    }

    result.line_count = relative_line;
    return result;
}

} // namespace mtfind