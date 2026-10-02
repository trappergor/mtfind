/**
 * @file test_searcher.cpp
 * @brief Юнит-тесты для поиска вхождений (точный поиск).
 */

#include "searcher.hpp"
#include "test_util.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

/// Хелпер: список позиций найденных вхождений.
std::vector<std::size_t> positions(const mtfind::Searcher& s,
                                   const std::string& line) {
    std::vector<std::size_t> out;
    for (const auto& m : s.find_all(line)) {
        out.push_back(m.position);
    }
    return out;
}

void test_simple_hits() {
    mtfind::Searcher s("bad");
    auto p = positions(s, "I've had my share of sand kicked in my face");
    test::check(p.empty());

    auto p2 = positions(s, "bad is bad");
    test::check_eq(p2.size(), std::size_t(2));
    if (p2.size() == 2) {
        test::check_eq(p2[0], std::size_t(0));
        test::check_eq(p2[1], std::size_t(7));
    }
}

void test_case_sensitive() {
    mtfind::Searcher s("Bad");
    auto p = positions(s, "bad Bad BAD");
    test::check_eq(p.size(), std::size_t(1));
    if (!p.empty()) {
        test::check_eq(p[0], std::size_t(4));
    }
}

void test_no_overlap() {
    // "aaa" в "aaaaa": только одно вхождение с позиции 0.
    mtfind::Searcher s("aaa");
    auto p = positions(s, "aaaaa");
    test::check_eq(p.size(), std::size_t(1));
    if (!p.empty()) {
        test::check_eq(p[0], std::size_t(0));
    }
}

void test_at_boundaries() {
    mtfind::Searcher s("abc");

    auto p1 = positions(s, "abc");
    test::check_eq(p1.size(), std::size_t(1));
    if (!p1.empty()) test::check_eq(p1[0], std::size_t(0));

    auto p2 = positions(s, "xabcx");
    test::check_eq(p2.size(), std::size_t(1));
    if (!p2.empty()) test::check_eq(p2[0], std::size_t(1));

    auto p3 = positions(s, "xxabc");
    test::check_eq(p3.size(), std::size_t(1));
    if (!p3.empty()) test::check_eq(p3[0], std::size_t(2));
}

void test_mask_longer_than_line() {
    mtfind::Searcher s("abcdef");
    test::check(positions(s, "abc").empty());
}

void test_mask_length_one() {
    mtfind::Searcher s("a");
    auto p = positions(s, "banana");
    test::check_eq(p.size(), std::size_t(3));
    if (p.size() == 3) {
        test::check_eq(p[0], std::size_t(1));
        test::check_eq(p[1], std::size_t(3));
        test::check_eq(p[2], std::size_t(5));
    }
}

void test_empty_mask_throws() {
    bool thrown = false;
    try {
        mtfind::Searcher s("");
        (void)s;
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    test::check(thrown);
}

void test_match_text() {
    mtfind::Searcher s("abc");
    auto m = s.find_all("xxabcxx");
    test::check_eq(m.size(), std::size_t(1));
    if (!m.empty()) {
        test::check_eq(m[0].text, std::string("abc"));
        test::check_eq(m[0].position, std::size_t(2));
    }
}

} // namespace

int main() {
    test_simple_hits();
    test_case_sensitive();
    test_no_overlap();
    test_at_boundaries();
    test_mask_longer_than_line();
    test_mask_length_one();
    test_empty_mask_throws();
    test_match_text();
    return test::summary("test_searcher");
}