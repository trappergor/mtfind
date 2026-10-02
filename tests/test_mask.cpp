/**
 * @file test_mask.cpp
 * @brief Юнит-тесты для compile_mask().
 */

#include "mask.hpp"
#include "test_util.hpp"

#include <stdexcept>
#include <string>

namespace {

void test_no_wildcards() {
    auto c = mtfind::compile_mask("abc");
    test::check_eq(c.length, std::size_t(3));
    test::check_eq(c.segments.size(), std::size_t(1));
    if (!c.segments.empty()) {
        test::check_eq(c.segments[0].offset, std::size_t(0));
        test::check_eq(c.segments[0].text, std::string("abc"));
    }
    test::check_eq(c.anchor_index, std::size_t(0));
}

void test_all_wildcards() {
    auto c = mtfind::compile_mask("???");
    test::check_eq(c.length, std::size_t(3));
    test::check(c.segments.empty());
    test::check_eq(c.anchor_index, mtfind::no_anchor);
}

void test_prefix_wildcard() {
    // "?ad" -> один сегмент "ad" со смещением 1.
    auto c = mtfind::compile_mask("?ad");
    test::check_eq(c.segments.size(), std::size_t(1));
    if (!c.segments.empty()) {
        test::check_eq(c.segments[0].offset, std::size_t(1));
        test::check_eq(c.segments[0].text, std::string("ad"));
    }
    test::check_eq(c.anchor_index, std::size_t(0));
}

void test_middle_wildcard() {
    // "ab?cd" -> два сегмента по 2 символа. Ничья -> выбираем первый.
    auto c = mtfind::compile_mask("ab?cd");
    test::check_eq(c.segments.size(), std::size_t(2));
    if (c.segments.size() == 2) {
        test::check_eq(c.segments[0].offset, std::size_t(0));
        test::check_eq(c.segments[0].text, std::string("ab"));
        test::check_eq(c.segments[1].offset, std::size_t(3));
        test::check_eq(c.segments[1].text, std::string("cd"));
    }
    test::check_eq(c.anchor_index, std::size_t(0));
}

void test_longest_wins() {
    // "a?bcdef" -> якорь "bcdef" (индекс 1), а не "a".
    auto c = mtfind::compile_mask("a?bcdef");
    test::check_eq(c.segments.size(), std::size_t(2));
    test::check_eq(c.anchor_index, std::size_t(1));
    if (c.anchor_index != mtfind::no_anchor) {
        test::check_eq(c.segments[c.anchor_index].text, std::string("bcdef"));
        test::check_eq(c.segments[c.anchor_index].offset, std::size_t(2));
    }
}

void test_multiple_wildcards() {
    // "?a?a?" -> сегменты "a"@1 и "a"@3. Ничья -> индекс 0.
    auto c = mtfind::compile_mask("?a?a?");
    test::check_eq(c.segments.size(), std::size_t(2));
    if (c.segments.size() == 2) {
        test::check_eq(c.segments[0].offset, std::size_t(1));
        test::check_eq(c.segments[0].text, std::string("a"));
        test::check_eq(c.segments[1].offset, std::size_t(3));
        test::check_eq(c.segments[1].text, std::string("a"));
    }
    test::check_eq(c.anchor_index, std::size_t(0));
}

void test_empty_throws() {
    test::check_throws<std::invalid_argument>([] {
        (void)mtfind::compile_mask("");
    });
}

} // namespace

int main() {
    test_no_wildcards();
    test_all_wildcards();
    test_prefix_wildcard();
    test_middle_wildcard();
    test_longest_wins();
    test_multiple_wildcards();
    test_empty_throws();
    return test::summary("test_mask");
}