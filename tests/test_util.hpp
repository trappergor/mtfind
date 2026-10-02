/**
 * @file test_util.hpp
 * @brief Минимальный шаблонный харнесс для юнит-тестов.
 */

#pragma once

#include <iostream>
#include <source_location>
#include <string>

namespace test {

/// Глобальный счётчик проваленных проверок в текущем процессе.
inline int& failures() {
    static int f = 0;
    return f;
}

/**
 * @brief Печатает значение, если оно поддерживает operator<<,
 *        иначе печатает заглушку.
 */
template <typename T>
void print_value(std::ostream& os, const T& v) {
    if constexpr (requires { os << v; }) {
        os << v;
    } else {
        os << "<unprintable>";
    }
}

/**
 * @brief Проверяет условие. При провале печатает файл и строку вызова.
 * @return true, если условие истинно.
 */
inline bool check(bool cond,
                  std::source_location loc = std::source_location::current()) {
    if (cond) {
        return true;
    }
    std::cerr << loc.file_name() << ':' << loc.line()
              << ": CHECK failed\n";
    ++failures();
    return false;
}

/**
 * @brief Проверяет равенство двух значений.
 * @return true, если значения равны.
 */
template <typename A, typename B>
bool check_eq(const A& a, const B& b,
              std::source_location loc = std::source_location::current()) {
    if (a == b) {
        return true;
    }
    std::cerr << loc.file_name() << ':' << loc.line() << ": CHECK_EQ failed: ";
    print_value(std::cerr, a);
    std::cerr << " != ";
    print_value(std::cerr, b);
    std::cerr << '\n';
    ++failures();
    return false;
}

/**
 * @brief Печатает итог по набору тестов и возвращает код выхода процесса.
 * @param suite Имя набора тестов.
 * @return 0, если не было провалов, иначе 1.
 */
inline int summary(const char* suite) {
    if (failures() == 0) {
        std::cout << "[  OK  ] " << suite << '\n';
        return 0;
    }
    std::cerr << "[ FAIL ] " << suite
              << " (" << failures() << " failures)\n";
    return 1;
}

} // namespace test