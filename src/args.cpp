/**
 * @file args.cpp
 * @brief Реализация разбора аргументов командной строки.
 */

#include "args.hpp"

namespace mtfind {

std::string usage() {
    return "Usage: mtfind <file> \"<mask>\"";
}

std::optional<Args> parse_args(int argc, char** argv) {
    // Программа принимает строго два позиционных аргумента.
    if (argc != 3) {
        return std::nullopt;
    }

    Args args;
    args.filename = argv[1];
    args.mask = argv[2];

    // Пустые значения считаем ошибкой использования.
    if (args.filename.empty() || args.mask.empty()) {
        return std::nullopt;
    }

    return args;
}

} // namespace mtfind