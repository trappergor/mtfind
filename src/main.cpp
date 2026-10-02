#include "args.hpp"
#include "searcher.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

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

} // namespace

int main(int argc, char** argv) {
    const auto parsed = mtfind::parse_args(argc, argv);
    if (!parsed) {
        std::cerr << mtfind::usage() << '\n';
        return 1;
    }

    std::ifstream in(parsed->filename, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file: " << parsed->filename << '\n';
        return 2;
    }

    mtfind::Searcher searcher(parsed->mask);

    std::vector<OutputItem> items;
    std::string line;
    std::size_t line_no = 0;

    while (std::getline(in, line)) {
        ++line_no;
        strip_cr(line);

        for (const auto& m : searcher.find_all(line)) {
            items.push_back(OutputItem{line_no, m.position + 1, m.text});
        }
    }

    // Формат вывода: сначала количество, затем сами вхождения.
    std::cout << items.size() << '\n';
    for (const auto& it : items) {
        std::cout << it.line_no << ' ' << it.position << ' ' << it.text << '\n';
    }

    return 0;
}