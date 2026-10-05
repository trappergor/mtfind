#include "args.hpp"
#include "file_splitter.hpp"
#include "searcher.hpp"
#include "worker.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fs = std::filesystem;

std::size_t pick_thread_count() {
    const auto n = std::thread::hardware_concurrency();
    return n == 0 ? 1 : static_cast<std::size_t>(n);
}

/**
 * @brief Проверяет, что путь указывает на доступный регулярный файл.
 */
bool check_input_file(const std::string& path,
                      std::size_t& size,
                      std::string& error) {
    std::error_code ec;
    const auto status = fs::status(path, ec);
    if (ec) {
        error = "cannot access file: " + path + " (" + ec.message() + ")";
        return false;
    }
    if (!fs::is_regular_file(status)) {
        error = "not a regular file: " + path;
        return false;
    }
    const auto sz = fs::file_size(path, ec);
    if (ec) {
        error = "cannot read file size: " + path + " (" + ec.message() + ")";
        return false;
    }
    size = static_cast<std::size_t>(sz);
    return true;
}

/// Читает uint64 из потока (little-endian).
bool read_u64(std::istream& is, std::uint64_t& v) {
    unsigned char buf[8];
    if (!is.read(reinterpret_cast<char*>(buf), sizeof(buf))) {
        return false;
    }
    v = 0;
    for (int i = 7; i >= 0; --i) {
        v = (v << 8) | buf[i];
    }
    return true;
}

/**
 * @brief RAII-уборщик временных файлов.
 *
 * Удаляет все перечисленные пути при разрушении, даже если main
 * завершился исключением.
 */
struct TempCleanup {
    std::vector<fs::path> paths;
    ~TempCleanup() {
        for (const auto& p : paths) {
            std::error_code ec;
            fs::remove(p, ec);
        }
    }
};

} // namespace

int main(int argc, char** argv) {
    const auto parsed = mtfind::parse_args(argc, argv);
    if (!parsed) {
        std::cerr << mtfind::usage() << '\n';
        return 1;
    }

    std::size_t size = 0;
    std::string err;
    if (!check_input_file(parsed->filename, size, err)) {
        std::cerr << err << '\n';
        return 2;
    }
    if (size == 0) {
        std::cout << 0 << '\n';
        return 0;
    }

    mtfind::Searcher searcher(parsed->mask);

    const std::size_t threads = pick_thread_count();
    const auto ranges = mtfind::split_file(parsed->filename, threads);
    const auto temp_dir = fs::temp_directory_path();

    std::vector<mtfind::WorkerResult> results(ranges.size());
    std::vector<std::thread> pool;
    pool.reserve(ranges.size());

    std::exception_ptr first_error = nullptr;
    std::mutex err_mutex;

    for (std::size_t i = 0; i < ranges.size(); ++i) {
        pool.emplace_back([&, i] {
            try {
                results[i] = mtfind::run_worker(
                    parsed->filename, ranges[i], searcher, temp_dir);
            } catch (...) {
                std::lock_guard<std::mutex> lk(err_mutex);
                if (!first_error) first_error = std::current_exception();
            }
        });
    }

    for (auto& t : pool) {
        t.join();
    }

    // Регистрируем пути к temp-файлам сразу после join(): даже если дальше
    // что-то бросит исключение, файлы будут удалены.
    TempCleanup cleanup;
    cleanup.paths.reserve(results.size());
    for (const auto& r : results) {
        if (!r.temp_path.empty()) {
            cleanup.paths.push_back(r.temp_path);
        }
    }

    if (first_error) {
        std::rethrow_exception(first_error);
    }

    std::size_t total = 0;
    for (const auto& r : results) {
        total += r.match_count;
    }
    std::cout << total << '\n';

    // Слияние: для каждого диапазона читаем бинарный temp-файл.
    // Вхождения внутри диапазона уже идут в порядке файла.
    std::size_t base_line = 0;
    for (const auto& r : results) {
        std::ifstream in(r.temp_path, std::ios::binary);
        std::uint64_t rel_line = 0;
        std::uint64_t pos = 0;
        std::uint64_t len = 0;
        while (read_u64(in, rel_line)
               && read_u64(in, pos)
               && read_u64(in, len)) {
            std::string text(len, '\0');
            in.read(text.data(), static_cast<std::streamsize>(len));
            std::cout << (base_line + rel_line) << ' '
                      << (pos + 1) << ' '
                      << text << '\n';
        }
        base_line += r.line_count;
    }

    return 0;
}