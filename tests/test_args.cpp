/**
 * @file test_args.cpp
 * @brief Юнит-тесты для разбора аргументов командной строки.
 */

#include "args.hpp"
#include "test_util.hpp"

#include <optional>
#include <string>
#include <vector>

namespace {

/// Вспомогательный вызов parse_args с вектором строк.
std::optional<mtfind::Args> call(std::vector<std::string> args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& s : args) {
        argv.push_back(s.data());
    }
    return mtfind::parse_args(static_cast<int>(argv.size()), argv.data());
}

void test_ok() {
    auto a = call({"mtfind", "input.txt", "?ad"});
    test::check(a.has_value());
    if (a) {
        test::check_eq(a->filename, std::string("input.txt"));
        test::check_eq(a->mask, std::string("?ad"));
    }
}

void test_missing_args() {
    test::check(!call({"mtfind"}).has_value());
    test::check(!call({"mtfind", "input.txt"}).has_value());
}

void test_extra_args() {
    test::check(!call({"mtfind", "input.txt", "?ad", "extra"}).has_value());
}

void test_empty() {
    test::check(!call({"mtfind", "", "?ad"}).has_value());
    test::check(!call({"mtfind", "input.txt", ""}).has_value());
}

void test_mask_with_newline_rejected() {
    test::check(!call({"mtfind", "input.txt", "a\nb"}).has_value());
    test::check(!call({"mtfind", "input.txt", "\n"}).has_value());
    test::check(call({"mtfind", "input.txt", "a\rb"}).has_value());
}

} // namespace

int main() {
    test_ok();
    test_missing_args();
    test_extra_args();
    test_empty();
    test_mask_with_newline_rejected();
    return test::summary("test_args");
}