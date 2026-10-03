/**
 * @file test_file_splitter.cpp
 * @brief Юнит-тесты для split_file().
 *
 * Тесты работают с временными файлами в temp-директории.
 */

#include "file_splitter.hpp"
#include "test_util.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

namespace fs = std::filesystem;

/**
 * @brief RAII-обёртка для временного файла: создаёт при конструировании,
 *        удаляет при разрушении.
 */
class TempFile {
public:
    explicit TempFile(const std::string& content) {
        static std::size_t counter = 0;
        path_ = fs::temp_directory_path()
              / ("mtfind_test_" + std::to_string(::getpid()) + "_"
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

/// Проверяет, что суммарная длина диапазонов == размер файла.
std::size_t total_bytes(const std::vector<mtfind::Range>& rs) {
    std::size_t s = 0;
    for (const auto& r : rs) s += (r.end - r.start);
    return s;
}

void test_empty_file() {
    TempFile f("");
    auto rs = mtfind::split_file(f.path().string(), 4);
    test::check(rs.empty());
}

void test_single_part() {
    TempFile f("line1\nline2\nline3\n");
    auto rs = mtfind::split_file(f.path().string(), 1);
    test::check_eq(rs.size(), std::size_t(1));
    if (!rs.empty()) {
        test::check_eq(rs[0].start, std::size_t(0));
        test::check_eq(rs[0].end, std::size_t(18));
    }
}

void test_many_parts_one_line_each() {
    // Файл: 5 строк по 6 байт ("lineN\n"). При 4 частях все диапазоны
    // непустые, границы — на началах строк.
    TempFile f("line1\nline2\nline3\nline4\nline5\n");
    auto rs = mtfind::split_file(f.path().string(), 4);

    test::check(!rs.empty());
    test::check_eq(total_bytes(rs), std::size_t(30));
    test::check_eq(rs.front().start, std::size_t(0));
    test::check_eq(rs.back().end, std::size_t(30));

    // Стыки: end[i] == start[i+1].
    for (std::size_t i = 0; i + 1 < rs.size(); ++i) {
        test::check_eq(rs[i].end, rs[i + 1].start);
    }

    // Каждый диапазон начинается либо с 0, либо сразу после '\n'.
    std::ifstream in(f.path(), std::ios::binary);
    for (const auto& r : rs) {
        if (r.start == 0) continue;
        in.seekg(static_cast<std::streamoff>(r.start - 1), std::ios::beg);
        char c = 0;
        in.get(c);
        test::check_eq(c, '\n');
    }
}

void test_more_parts_than_lines() {
    TempFile f("a\nb\n");
    auto rs = mtfind::split_file(f.path().string(), 8);
    // Ноль диапазонов быть не может, но меньше 8 — нормально.
    test::check(!rs.empty());
    test::check(rs.size() <= 8);
    test::check_eq(total_bytes(rs), std::size_t(4));
    test::check_eq(rs.front().start, std::size_t(0));
    test::check_eq(rs.back().end, std::size_t(4));
}

void test_no_trailing_newline() {
    TempFile f("abc");
    auto rs = mtfind::split_file(f.path().string(), 4);
    test::check_eq(rs.size(), std::size_t(1));
    if (!rs.empty()) {
        test::check_eq(rs[0].start, std::size_t(0));
        test::check_eq(rs[0].end, std::size_t(3));
    }
}

void test_zero_parts_throws() {
    TempFile f("abc");
    test::check_throws<std::runtime_error>([&] {
        (void)mtfind::split_file(f.path().string(), 0);
    });
}

void test_missing_file_throws() {
    test::check_throws<std::runtime_error>([] {
        (void)mtfind::split_file("/nonexistent/path/xyz.txt", 4);
    });
}

} // namespace

int main() {
    test_empty_file();
    test_single_part();
    test_many_parts_one_line_each();
    test_more_parts_than_lines();
    test_no_trailing_newline();
    test_zero_parts_throws();
    test_missing_file_throws();
    return test::summary("test_file_splitter");
}