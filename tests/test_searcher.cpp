/**
 * @file test_searcher.cpp
 * @brief Юнит-тесты для поиска вхождений (точный и с '?').
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

// ------------------------------- exact ------------------------------------

void test_simple_hits() {
    mtfind::Searcher s("bad");
    test::check(positions(s, "I've had my share of sand kicked in my face")
                    .empty());

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

void test_no_overlap_exact() {
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
    test::check_throws<std::invalid_argument>([] {
        (void)mtfind::Searcher("");
    });
}

void test_match_text_exact() {
    mtfind::Searcher s("abc");
    auto m = s.find_all("xxabcxx");
    test::check_eq(m.size(), std::size_t(1));
    if (!m.empty()) {
        test::check_eq(m[0].text, std::string("abc"));
        test::check_eq(m[0].position, std::size_t(2));
    }
}

// ------------------------------ wildcards ---------------------------------

void test_wildcard_prefix() {
    // "?ad" на "bad mad had".
    mtfind::Searcher s("?ad");
    auto m = s.find_all("bad mad had");
    test::check_eq(m.size(), std::size_t(3));
    if (m.size() == 3) {
        test::check_eq(m[0].position, std::size_t(0));
        test::check_eq(m[0].text, std::string("bad"));
        test::check_eq(m[1].position, std::size_t(4));
        test::check_eq(m[1].text, std::string("mad"));
        test::check_eq(m[2].position, std::size_t(8));
        test::check_eq(m[2].text, std::string("had"));
    }
}

void test_wildcard_middle() {
    // "a?c" на "abc axc a c" (последнее совпадение — a-probel-c).
    mtfind::Searcher s("a?c");
    auto m = s.find_all("abc axc a c");
    test::check_eq(m.size(), std::size_t(3));
    if (m.size() == 3) {
        test::check_eq(m[0].position, std::size_t(0));
        test::check_eq(m[0].text, std::string("abc"));
        test::check_eq(m[1].position, std::size_t(4));
        test::check_eq(m[1].text, std::string("axc"));
        test::check_eq(m[2].position, std::size_t(8));
        test::check_eq(m[2].text, std::string("a c"));
    }
}

void test_all_wildcards() {
    mtfind::Searcher s("??");
    auto m = s.find_all("abcde");
    test::check_eq(m.size(), std::size_t(2));
    if (m.size() == 2) {
        test::check_eq(m[0].position, std::size_t(0));
        test::check_eq(m[0].text, std::string("ab"));
        test::check_eq(m[1].position, std::size_t(2));
        test::check_eq(m[1].text, std::string("cd"));
    }
}

void test_all_wildcards_length_one() {
    mtfind::Searcher s("?");
    auto m = s.find_all("abc");
    test::check_eq(m.size(), std::size_t(3));
    if (m.size() == 3) {
        test::check_eq(m[0].position, std::size_t(0));
        test::check_eq(m[1].position, std::size_t(1));
        test::check_eq(m[2].position, std::size_t(2));
    }
}

void test_mixed_wildcards_edges() {
    // "a?a?" на "abab".
    {
        mtfind::Searcher s("a?a?");
        auto m = s.find_all("abab");
        test::check_eq(m.size(), std::size_t(1));
        if (!m.empty()) {
            test::check_eq(m[0].position, std::size_t(0));
            test::check_eq(m[0].text, std::string("abab"));
        }
    }
    // "?a?a?" на "xayaz".
    {
        mtfind::Searcher s("?a?a?");
        auto m = s.find_all("xayaz");
        test::check_eq(m.size(), std::size_t(1));
        if (!m.empty()) {
            test::check_eq(m[0].position, std::size_t(0));
            test::check_eq(m[0].text, std::string("xayaz"));
        }
    }
}

void test_wildcard_no_overlap() {
    // "?a" на "aaa": только одно вхождение "aa" с позиции 0.
    mtfind::Searcher s("?a");
    auto m = s.find_all("aaa");
    test::check_eq(m.size(), std::size_t(1));
    if (!m.empty()) {
        test::check_eq(m[0].position, std::size_t(0));
        test::check_eq(m[0].text, std::string("aa"));
    }

    // "?a" на "aaaa": "aa" на позиции 0 и "aa" на позиции 2.
    auto m2 = s.find_all("aaaa");
    test::check_eq(m2.size(), std::size_t(2));
    if (m2.size() == 2) {
        test::check_eq(m2[0].position, std::size_t(0));
        test::check_eq(m2[1].position, std::size_t(2));
    }
}

void test_wildcard_mask_longer_than_line() {
    mtfind::Searcher s("?abcd");
    test::check(positions(s, "abc").empty());
}

void test_wildcard_mismatch() {
    // "?ad" на "bed": литеральный суффикс не совпадает.
    mtfind::Searcher s("?ad");
    test::check(positions(s, "bed").empty());
}

void test_spec_example_line5() {
    // Строка 5 из примера: "And bad mistakes ?".
    // Маска "?ad" должна найти "bad" на позиции 4 (0-based).
    mtfind::Searcher s("?ad");
    auto m = s.find_all("And bad mistakes ?");
    test::check_eq(m.size(), std::size_t(1));
    if (!m.empty()) {
        test::check_eq(m[0].position, std::size_t(4));
        test::check_eq(m[0].text, std::string("bad"));
    }
}

void test_spec_example_line6() {
    // Строка 6 из примера: "I've made a few.".
    // Маска "?ad" должна найти "mad" на позиции 5 (0-based).
    mtfind::Searcher s("?ad");
    auto m = s.find_all("I've made a few.");
    test::check_eq(m.size(), std::size_t(1));
    if (!m.empty()) {
        test::check_eq(m[0].position, std::size_t(5));
        test::check_eq(m[0].text, std::string("mad"));
    }
}

void test_spec_example_line7() {
    // Строка 7 из примера: "I've had my share of sand kicked in my face".
    // Маска "?ad" должна найти "had" на позиции 5 (0-based).
    mtfind::Searcher s("?ad");
    auto m = s.find_all("I've had my share of sand kicked in my face");
    test::check_eq(m.size(), std::size_t(1));
    if (!m.empty()) {
        test::check_eq(m[0].position, std::size_t(5));
        test::check_eq(m[0].text, std::string("had"));
    }
}

} // namespace

int main() {
    // exact
    test_simple_hits();
    test_case_sensitive();
    test_no_overlap_exact();
    test_at_boundaries();
    test_mask_longer_than_line();
    test_mask_length_one();
    test_empty_mask_throws();
    test_match_text_exact();

    // wildcards
    test_wildcard_prefix();
    test_wildcard_middle();
    test_all_wildcards();
    test_all_wildcards_length_one();
    test_mixed_wildcards_edges();
    test_wildcard_no_overlap();
    test_wildcard_mask_longer_than_line();
    test_wildcard_mismatch();
    test_spec_example_line5();
    test_spec_example_line6();
    test_spec_example_line7();

    return test::summary("test_searcher");
}