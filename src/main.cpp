#include "args.hpp"
#include "file_splitter.hpp"
#include "searcher.hpp"
#include "worker.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fs = std::filesystem;

/**
 * @brief Внутреннее представление вхождения с глобальными координатами.
 */
struct OutputItem {
    std::size_t line_no = 0;  ///< Номер строки (1-based).
    std::size_t position = 0; ///< Позиция в строке (1-based).
    std::string text;         ///< Найденная подстрока.
};

/**
 * @brief Убирает завершающий '\r' (для файлов с CRLF).
 */
void strip_cr(std::string& line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

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

    // Каждый диапазон обрабатывается своим потоком. Результаты пишутся
    // в отдельные временные файлы, чтобы избежать синхронизации при выводе.
    std::vector<mtfind::WorkerResult> results(ranges.size());
    std::vector<std::thread> pool;
    pool.reserve(ranges.size());

    std::exception_ptr first_error = nullptr;
    std::mutex err_mutex;

    for (std::size_t i = 0; i < ranges.size(); ++i) {
        pool.emplace_back([&, i] {
            try {
                results[i] = mtfind::run_worker(
                    parsed->filename, ranges[i], searcher,
                    fs::temp_directory_path());
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

    // Префиксная сумма обработанных строк: базовый номер строки для
    // диапазона i — сумма line_count всех предыдущих диапазонов.
    std::vector<std::size_t> base_line(ranges.size(), 0);
    std::size_t total_matches = 0;
    for (std::size_t i = 0; i < results.size(); ++i) {
        if (i > 0) {
            base_line[i] = base_line[i - 1] + results[i - 1].line_count;
        }
        total_matches += results[i].match_count;
    }

    std::cout << total_matches << '\n';

    // Слияние: диапазоны в порядке файла, строки внутри диапазона тоже
    // в порядке файла (воркер пишет их последовательно). Глобальный
    // номер строки = base_line + relative_line.
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& res = results[i];
        std::ifstream in(res.temp_path);
        std::string line;
        while (std::getline(in, line)) {
            // Формат строки: "<relative_line> <position> <text>".
            std::istringstream iss(line);
            std::size_t rel = 0;
            std::size_t pos = 0;
            iss >> rel >> pos;
            // Всё, что после второго пробела — текст вхождения.
            std::string text;
            std::getline(iss, text);
            if (!text.empty() && text.front() == ' ') {
                text.erase(text.begin());
            }
            const std::size_t global_line = base_line[i] + rel;
            std::cout << global_line << ' ' << (pos + 1) << ' ' << text << '\n';
        }
    }

    // Уборка временных файлов.
    for (const auto& res : results) {
        std::error_code ec;
        fs::remove(res.temp_path, ec);
    }

    return 0;
}