#include "args.hpp"
#include "file_splitter.hpp"
#include "searcher.hpp"
#include "worker.hpp"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fs = std::filesystem;

/**
 * @brief Сколько потоков использовать.
 *  
 * Берём hardware_concurrency, но не меньше 1.
 */
std::size_t pick_thread_count() {
    const auto n = std::thread::hardware_concurrency();
    return n == 0 ? 1 : static_cast<std::size_t>(n);
}

/**
 * @brief Проверяет, что путь указывает на доступный регулярный файл.
 *
 * @param path Путь к файлу.
 * @param[out] size Размер файла в байтах при успехе.
 * @param[out] error Сообщение об ошибке при неуспехе.
 * @return true, если файл доступен и является регулярным.
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
        // Пустой файл — валидный вход, вхождений нет.
        std::cout << 0 << '\n';
        return 0;
    }

    mtfind::Searcher searcher(parsed->mask);

    const std::size_t threads = pick_thread_count();
    const auto ranges = mtfind::split_file(parsed->filename, threads);

    // Каждый диапазон обрабатывается своим потоком.
    std::vector<mtfind::WorkerResult> results(ranges.size());
    std::vector<std::thread> pool;
    pool.reserve(ranges.size());

    std::exception_ptr first_error = nullptr;
    std::mutex err_mutex;

    for (std::size_t i = 0; i < ranges.size(); ++i) {
        pool.emplace_back([&, i] {
            try {
                results[i] = mtfind::run_worker(
                    parsed->filename, ranges[i], searcher);
            } catch (...) {
                std::lock_guard<std::mutex> lk(err_mutex);
                if (!first_error) first_error = std::current_exception();
            }
        });
    }

    for (auto& t : pool) {
        t.join();
    }

    if (first_error) {
        std::rethrow_exception(first_error);
    }

    // Считаем общее количество вхождений до вывода — формат требует
    // напечатать его первой строкой.
    std::size_t total = 0;
    for (const auto& r : results) {
        total += r.matches.size();
    }
    std::cout << total << '\n';

    // Слияние в порядке диапазонов. Внутри диапазона вхождения уже идут
    // в порядке строк и позиций (воркер обрабатывает строки последовательно).
    // Глобальный номер строки = сумма line_count предыдущих диапазонов
    // плюс относительный номер.
    std::size_t base_line = 0;
    for (const auto& r : results) {
        for (const auto& m : r.matches) {
            std::cout << (base_line + m.line) << ' '
                      << (m.position + 1) << ' '
                      << m.text << '\n';
        }
        base_line += r.line_count;
    }

    return 0;
}