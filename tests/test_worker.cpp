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

namespace {

namespace fs = std::filesystem;

/// RAII-обёртка для временного входного файла.
class TempFile {
public:
    explicit TempFile(const std::string& content) {
        static std::size_t counter = 0;
        path_ = fs::temp_directory_path()
              / ("mtfind_worker_in_" + std::to_string(counter++) + ".txt");
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

void test_full_range_three_hits() {
    TempFile f("bad mad had\n");
    mtfind::Searcher s("?ad");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 12},
                                  s);

    test::check_eq(res.line_count, std::size_t(1));
    test::check_eq(res.matches.size(), std::size_t(3));
    if (res.matches.size() == 3) {
        test::check_eq(res.matches[0].line, std::size_t(1));
        test::check_eq(res.matches[0].position, std::size_t(0));
        test::check_eq(res.matches[0].text, std::string("bad"));
        test::check_eq(res.matches[1].position, std::size_t(4));
        test::check_eq(res.matches[1].text, std::string("mad"));
        test::check_eq(res.matches[2].position, std::size_t(8));
        test::check_eq(res.matches[2].text, std::string("had"));
    }
}

void test_relative_line_numbers() {
    // Файл: "aaaa\nbbb\nccc\n" (13 байт). Границы строк:
    //   [0, 5)   -> "aaaa\n"
    //   [5, 9)   -> "bbb\n"
    //   [9, 13)  -> "ccc\n"
    //
    // Проверяем, что воркер считает relative_line от начала диапазона,
    // а не от начала файла. Диапазоны задаём явно, чтобы не зависеть
    // от того, куда именно split_file() положит границу.
    TempFile f("aaaa\nbbb\nccc\n");

    {
        mtfind::Searcher s("bbb");
        auto res = mtfind::run_worker(f.path().string(),
                                      mtfind::Range{5, 9},
                                      s);
        test::check_eq(res.line_count, std::size_t(1));
        test::check_eq(res.matches.size(), std::size_t(1));
        if (!res.matches.empty()) {
            test::check_eq(res.matches[0].line, std::size_t(1));
            test::check_eq(res.matches[0].position, std::size_t(0));
            test::check_eq(res.matches[0].text, std::string("bbb"));
        }
    }

    {
        mtfind::Searcher s("ccc");
        auto res = mtfind::run_worker(f.path().string(),
                                      mtfind::Range{9, 13},
                                      s);
        test::check_eq(res.line_count, std::size_t(1));
        test::check_eq(res.matches.size(), std::size_t(1));
        if (!res.matches.empty()) {
            test::check_eq(res.matches[0].line, std::size_t(1));
            test::check_eq(res.matches[0].position, std::size_t(0));
            test::check_eq(res.matches[0].text, std::string("ccc"));
        }
    }
}

void test_leading_space_is_preserved() {
    // Маска "?a" должна найти " a" в строке "xx a".
    // Вхождение начинается с пробела — он должен сохраниться в m.text.
    TempFile f("xx a\n");
    mtfind::Searcher s("?a");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 5},
                                  s);
    test::check_eq(res.matches.size(), std::size_t(1));
    if (!res.matches.empty()) {
        test::check_eq(res.matches[0].position, std::size_t(2));
        test::check_eq(res.matches[0].text, std::string(" a"));
    }
}

void test_leading_space_preserved_exact() {
    // Точная маска " ab" в "xx aby" — вхождение с ведущим пробелом.
    TempFile f("xx aby\n");
    mtfind::Searcher s(" ab");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 7},
                                  s);
    test::check_eq(res.matches.size(), std::size_t(1));
    if (!res.matches.empty()) {
        test::check_eq(res.matches[0].position, std::size_t(2));
        test::check_eq(res.matches[0].text, std::string(" ab"));
    }
}

void test_empty_range() {
    TempFile f("abc\n");
    mtfind::Searcher s("abc");

    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{5, 5},
                                  s);
    test::check_eq(res.line_count, std::size_t(0));
    test::check(res.matches.empty());
}

void test_no_matches() {
    TempFile f("xyz xyz xyz\n");
    mtfind::Searcher s("?ad");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 12},
                                  s);
    test::check_eq(res.line_count, std::size_t(1));
    test::check(res.matches.empty());
}

void test_missing_input_file_throws() {
    mtfind::Searcher s("abc");
    test::check_throws<std::runtime_error>([] {
        (void)mtfind::run_worker("/nonexistent/xyz.txt",
                                 mtfind::Range{0, 10},
                                 mtfind::Searcher("abc"));
    });
}

} // namespace

int main() {
    test_full_range_three_hits();
    test_relative_line_numbers();
    test_leading_space_is_preserved();
    test_leading_space_preserved_exact();
    test_empty_range();
    test_no_matches();
    test_missing_input_file_throws();
    return test::summary("test_worker");
}