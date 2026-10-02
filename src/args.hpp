/**
 * @file args.hpp
 * @brief Разбор аргументов командной строки программы mtfind.
 */

#pragma once

#include <optional>
#include <string>

namespace mtfind {

/**
 * @brief Параметры, переданные программе через командную строку.
 */
struct Args {
    std::string filename; ///< Путь к файлу, в котором производится поиск.
    std::string mask;     ///< Маска поиска (может содержать '?').
};

/**
 * @brief Разбирает аргументы командной строки.
 *
 * Ожидается ровно два позиционных аргумента: имя файла и маска.
 * Маска берётся как единый аргумент (в оболочке её нужно передавать в кавычках).
 *
 * @param argc Количество аргументов (как в main).
 * @param argv Массив аргументов (как в main).
 * @return Разобранные аргументы, либо std::nullopt, если аргументы некорректны.
 */
std::optional<Args> parse_args(int argc, char** argv);

/**
 * @brief Возвращает строку с краткой справкой по использованию.
 */
std::string usage();

} // namespace mtfind