/**
 * @file worker.cpp
 * @brief Реализация обработки одного диапазона.
 */

#include "worker.hpp"

#include <atomic>
#include <chrono>
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

/**
 * @brief Записывает uint64 в little-endian.
 *
 * Порядок байт фиксируем вручную, чтобы формат не зависел от архитектуры.
 */
void write_u64(std::ostream& os, std::uint64_t v) {
    char buf[8];
    for (int i = 0; i < 8; ++i) {
        buf[i] = static_cast<char>(v & 0xFFu);
        v >>= 8;
    }
    os.write(buf, sizeof(buf));
}

/**
 * @brief RAII-хранитель, удаляющий файл при разрушении.
 */
class TempFileGuard {
public:
    explicit TempFileGuard(std::filesystem::path p)
        : path_(std::move(p)) {}

    ~TempFileGuard() {
        if (!dismissed_) {
            std::error_code ec;
            std::filesystem::remove(path_, ec);
        }
    }

    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;

    /// Отключает удаление — файл остаётся на диске.
    void dismiss() noexcept { dismissed_ = true; }

private:
    std::filesystem::path path_;
    bool dismissed_ = false;
};

} // namespace

std::filesystem::path make_temp_path(const std::filesystem::path& dir) {
    static std::atomic<std::size_t> counter{0};
    const auto id = counter.fetch_add(1, std::memory_order_relaxed);
    const auto ts = std::chrono::steady_clock::now()
                        .time_since_epoch()
                        .count();
    return dir / ("mtfind_part_" + std::to_string(ts) + "_"
                  + std::to_string(id) + ".bin");
}

WorkerResult run_worker(const std::string& filename,
                        const Range& range,
                        const Searcher& searcher,
                        const std::filesystem::path& temp_dir) {
    WorkerResult result;
    result.temp_path = make_temp_path(temp_dir);

    // Пустой диапазон: файл создаём (для единообразия), но ничего не пишем.
    if (range.end <= range.start) {
        std::ofstream{result.temp_path, std::ios::binary};
        return result;
    }

    std::ofstream out(result.temp_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot create temp file: "
                                 + result.temp_path.string());
    }

    TempFileGuard guard(result.temp_path);

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file: " + filename);
    }
    in.seekg(static_cast<std::streamoff>(range.start), std::ios::beg);

    std::size_t relative_line = 0;
    std::string line;

    // split_file гарантирует, что вся строка целиком лежит в [start, end),
    // поэтому цикл не перескакивает границу диапазона.
    while (static_cast<std::size_t>(in.tellg()) < range.end) {
        if (!std::getline(in, line)) {
            break;
        }
        ++relative_line;
        strip_cr(line);

        for (const auto& m : searcher.find_all(line)) {
            // Формат: <line><position><length><bytes>
            write_u64(out, relative_line);
            write_u64(out, m.position);
            write_u64(out, m.text.size());
            out.write(m.text.data(),
                      static_cast<std::streamsize>(m.text.size()));
            ++result.match_count;
        }
    }

    result.line_count = relative_line;
    guard.dismiss();
    return result;
}

} // namespace mtfind