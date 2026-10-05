/**
 * @file test_worker.cpp
 * @brief Юнит-тесты для run_worker().
 */

#include "file_splitter.hpp"
#include "searcher.hpp"
#include "test_util.hpp"
#include "worker.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

/// RAII-обёртка для временного входного файла.
class TempFile {
public:
    explicit TempFile(const std::string& content) {
        static std::atomic<std::size_t> counter{0};
        const auto id = counter.fetch_add(1);
        const auto ts = std::chrono::steady_clock::now()
                            .time_since_epoch().count();
        path_ = fs::temp_directory_path()
              / ("mtfind_worker_in_" + std::to_string(ts) + "_"
                 + std::to_string(id) + ".txt");
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

struct RawMatch {
    std::uint64_t line = 0;
    std::uint64_t position = 0;
    std::string text;
};

/// Читает uint64 в little-endian.
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

std::vector<RawMatch> read_matches(const fs::path& p) {
    std::vector<RawMatch> out;
    std::ifstream in(p, std::ios::binary);
    while (true) {
        RawMatch m;
        std::uint64_t len = 0;
        if (!read_u64(in, m.line)) break;
        if (!read_u64(in, m.position)) break;
        if (!read_u64(in, len)) break;
        m.text.assign(static_cast<std::size_t>(len), '\0');
        in.read(m.text.data(), static_cast<std::streamsize>(len));
        out.push_back(std::move(m));
    }
    return out;
}

void test_full_range_three_hits() {
    TempFile f("bad mad had\n");
    mtfind::Searcher s("?ad");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 12},
                                  s,
                                  fs::temp_directory_path());

    test::check_eq(res.line_count, std::size_t(1));
    test::check_eq(res.match_count, std::size_t(3));

    auto ms = read_matches(res.temp_path);
    test::check_eq(ms.size(), std::size_t(3));
    if (ms.size() == 3) {
        test::check_eq(ms[0].line, std::uint64_t(1));
        test::check_eq(ms[0].position, std::uint64_t(0));
        test::check_eq(ms[0].text, std::string("bad"));
        test::check_eq(ms[1].position, std::uint64_t(4));
        test::check_eq(ms[1].text, std::string("mad"));
        test::check_eq(ms[2].position, std::uint64_t(8));
        test::check_eq(ms[2].text, std::string("had"));
    }

    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_relative_line_numbers() {
    // Файл: "aaaa\nbbb\nccc\n" (13 байт). Границы строк:
    //   [0, 5)   -> "aaaa\n"
    //   [5, 9)   -> "bbb\n"
    //   [9, 13)  -> "ccc\n"
    // Задаём диапазоны явно: воркер считает relative_line от начала
    // диапазона, а не от начала файла.
    TempFile f("aaaa\nbbb\nccc\n");

    {
        mtfind::Searcher s("bbb");
        auto res = mtfind::run_worker(f.path().string(),
                                      mtfind::Range{5, 9},
                                      s,
                                      fs::temp_directory_path());
        test::check_eq(res.line_count, std::size_t(1));
        test::check_eq(res.match_count, std::size_t(1));
        auto ms = read_matches(res.temp_path);
        if (!ms.empty()) {
            test::check_eq(ms[0].line, std::uint64_t(1));
            test::check_eq(ms[0].text, std::string("bbb"));
        }
        std::error_code ec;
        fs::remove(res.temp_path, ec);
    }

    {
        mtfind::Searcher s("ccc");
        auto res = mtfind::run_worker(f.path().string(),
                                      mtfind::Range{9, 13},
                                      s,
                                      fs::temp_directory_path());
        test::check_eq(res.line_count, std::size_t(1));
        test::check_eq(res.match_count, std::size_t(1));
        auto ms = read_matches(res.temp_path);
        if (!ms.empty()) {
            test::check_eq(ms[0].line, std::uint64_t(1));
            test::check_eq(ms[0].text, std::string("ccc"));
        }
        std::error_code ec;
        fs::remove(res.temp_path, ec);
    }
}

void test_leading_space_is_preserved() {
    // Маска "?a" должна найти " a" в строке "xx a".
    TempFile f("xx a\n");
    mtfind::Searcher s("?a");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 5},
                                  s,
                                  fs::temp_directory_path());

    test::check_eq(res.match_count, std::size_t(1));
    auto ms = read_matches(res.temp_path);
    if (!ms.empty()) {
        test::check_eq(ms[0].position, std::uint64_t(2));
        test::check_eq(ms[0].text, std::string(" a"));
    }

    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_empty_range() {
    TempFile f("abc\n");
    mtfind::Searcher s("abc");

    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{5, 5},
                                  s,
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
    test::check_throws<std::runtime_error>([] {
        (void)mtfind::run_worker("/nonexistent/xyz.txt",
                                 mtfind::Range{0, 10},
                                 mtfind::Searcher("abc"),
                                 fs::temp_directory_path());
    });
}

void test_crlf_stripped() {
    // CRLF-строка: воркер должен снять '\r' перед поиском.
    TempFile f("abc\r\nxyz bad\r\nzzz\r\n");
    mtfind::Searcher s("?ad");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 17},
                                  s,
                                  fs::temp_directory_path());
    test::check_eq(res.line_count, std::size_t(3));
    test::check_eq(res.match_count, std::size_t(1));
    auto ms = read_matches(res.temp_path);
    if (!ms.empty()) {
        test::check_eq(ms[0].line, std::uint64_t(2));
        test::check_eq(ms[0].position, std::uint64_t(4));
        test::check_eq(ms[0].text, std::string("bad"));
    }
    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_empty_lines_are_counted() {
    // "a\n\nb\n": три строки, во второй пусто.
    TempFile f("a\n\nb\n");
    mtfind::Searcher s("?");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 5},
                                  s,
                                  fs::temp_directory_path());
    test::check_eq(res.line_count, std::size_t(3));
    test::check_eq(res.match_count, std::size_t(2));
    auto ms = read_matches(res.temp_path);
    if (ms.size() == 2) {
        test::check_eq(ms[0].line, std::uint64_t(1));
        test::check_eq(ms[0].text, std::string("a"));
        test::check_eq(ms[1].line, std::uint64_t(3));
        test::check_eq(ms[1].text, std::string("b"));
    }
    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_only_newlines_file() {
    // Файл из одних '\n'. Три строки, вхождений нет.
    TempFile f("\n\n\n");
    mtfind::Searcher s("a");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 3},
                                  s,
                                  fs::temp_directory_path());
    test::check_eq(res.line_count, std::size_t(3));
    test::check_eq(res.match_count, std::size_t(0));
    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

void test_blank_file() {
    TempFile f("");
    mtfind::Searcher s("a");
    auto res = mtfind::run_worker(f.path().string(),
                                  mtfind::Range{0, 0},
                                  s,
                                  fs::temp_directory_path());
    test::check_eq(res.line_count, std::size_t(0));
    test::check_eq(res.match_count, std::size_t(0));
    std::error_code ec;
    fs::remove(res.temp_path, ec);
}

} // namespace

int main() {
    test_full_range_three_hits();
    test_relative_line_numbers();
    test_leading_space_is_preserved();
    test_empty_range();
    test_no_matches();
    test_missing_input_file_throws();
    test_crlf_stripped();
    test_empty_lines_are_counted();
    test_only_newlines_file();
    test_blank_file();
    return test::summary("test_worker");
}