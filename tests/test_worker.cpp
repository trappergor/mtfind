/**
 * @file test_worker.cpp
 * @brief Юнит-тесты для run_worker().
 */

#include "file_splitter.hpp"
#include "searcher.hpp"
#include "test_util.hpp"
#include "worker.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

namespace fs = std::filesystem;

/// RAII-обёртка для временного входного файла.
class TempFile {
public:
    explicit TempFile(const std::string& content) {
        static std::size_t counter = 0;
        path_ = fs::temp_directory_path()
              / ("mtfind_worker_in_" + std::to_string(::getpid()) + "_"
                 + std::to_string(counter++) + ".txt");
        std::ofstream out(path_, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
    ~TempFile() {
        std::error_code ec;
        fs::remove(path_, ec);
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    const fs::path& path() const noexcept { return path_; }
private:
    fs::path path_;
};

/// Читает все строки из указанного файла.
std::vector<std::string> read_lines(const fs::path& p) {
    std::vector<std::string> lines;
    std::ifstream in(p);
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

void test_full_range_two_hits() {
    // Диапазон покрывает весь файл. Ищем "?ad" в "bad mad had".
    TempFile f("bad mad had\n");
    mtfind::Searcher s("?ad");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 12},
                                  s,
                                  fs::temp_directory_path());

    test::check_eq(res.line_count, std::size_t(1));
    test::check_eq(res.match_count, std::size_t(3));

    auto lines = read_lines(res.temp_path);
    test::check_eq(lines.size(), std::size_t(3));
    if (lines.size() == 3) {
        test::check_eq(lines[0], std::string("1 0 bad"));
        test::check_eq(lines[1], std::string("1 4 mad"));
        test::check_eq(lines[2], std::string("1 8 had"));
    }

    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_partial_range_line_numbers_are_relative() {
    // Файл: "aaaa\nbbb\nccc\n". Диапазон от начала второй строки до конца.
    // Строки 2 и 3 (в нумерации файла) внутри диапазона имеют
    // relative_line 1 и 2.
    const std::string content = "aaaa\nbbb\nccc\n";
    TempFile f(content);
    mtfind::Searcher s("bbb");

    auto rs = mtfind::split_file(f.path().string(), 3);
    test::check(!rs.empty());

    // Возьмём последний диапазон — он точно начинается после '\n'.
    const auto& r = rs.back();
    auto res = mtfind::run_worker(f.path().string(), r, s,
                                  fs::temp_directory_path());

    // Внутри последнего диапазона: 'bbb', 'ccc' (и, возможно, только 'ccc' —
    // зависит от того, куда попала граница). Но relative_line всегда 1-based.
    test::check(res.line_count >= 1);

    auto lines = read_lines(res.temp_path);
    for (const auto& l : lines) {
        // Проверяем формат "<line> <pos> <text>".
        auto first = l.find(' ');
        test::check_false(first == std::string::npos);
        test::check_eq(l.substr(0, first), std::string("1"));
    }

    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_empty_range() {
    TempFile f("abc\n");
    mtfind::Searcher s("abc");

    mtfind::Range empty{5, 5};
    auto res = mtfind::run_worker(f.path().string(), empty, s,
                                  fs::temp_directory_path());

    test::check_eq(res.line_count, std::size_t(0));
    test::check_eq(res.match_count, std::size_t(0));

    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_no_matches() {
    TempFile f("xyz xyz xyz\n");
    mtfind::Searcher s("?ad");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 12},
                                  s,
                                  fs::temp_directory_path());

    test::check_eq(res.line_count, std::size_t(1));
    test::check_eq(res.match_count, std::size_t(0));

    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_missing_input_file_throws() {
    mtfind::Searcher s("abc");
    test::check_throws<std::runtime_error>([] {
        (void)mtfind::run_worker("/nonexistent/xyz.txt",
                                 mtfind::Range{0, 10},
                                 mtfind::Searcher("abc"),
                                 fs::temp_directory_path());
    });
}

} // namespace

int main() {
    test_full_range_two_hits();
    test_partial_range_line_numbers_are_relative();
    test_empty_range();
    test_no_matches();
    test_missing_input_file_throws();
    return test::summary("test_worker");
}