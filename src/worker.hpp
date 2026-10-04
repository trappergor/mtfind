/**
 * @file worker.hpp
 * @brief Обработка одного байтового диапазона файла.
 * 
 * Воркер читает строки в диапазоне [start, end), ищет вхождения маски
 * и возвращает их в памяти. Номера строк внутри результата относительные
 * (1-based внутри диапазона); главный поток превращает их в глобальные,
 * суммируя line_count предыдущих диапазонов.
 *
 */

#pragma once

#include "file_splitter.hpp"
#include "searcher.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace mtfind {

/**
 * @brief Одно найденное вхождение внутри диапазона.
 *
 * Все поля хранят "сырые" данные без дополнительной интерпретации:
 * строка — относительный номер (1-based), позиция — 0-based, текст — как
 * он реально выглядит в файле (с ведущими пробелами, если они есть).
 */
struct MatchRecord {
    std::size_t line = 0;     ///< Номер строки внутри диапазона (1-based).
    std::size_t position = 0; ///< Позиция вхождения в строке (0-based).
    std::string text;         ///< Точный текст вхождения.
};

/**
 * @brief Результат работы воркера.
 */
struct WorkerResult {
    std::size_t line_count = 0;          ///< Сколько строк обработано.
    std::vector<MatchRecord> matches;    ///< Найденные вхождения в порядке файла.
};

/**
 * @brief Обрабатывает один диапазон файла.
 *
 * @param filename Путь к исходному файлу.
 * @param range Диапазон байт [start, end). Должен начинаться с начала строки.
 * @param searcher Поисковик с уже скомпилированной маской.
 * @return Статистика воркера: число строк и все вхождения.
 * @throw std::runtime_error если файл не открывается.
 */
WorkerResult run_worker(const std::string& filename,
                        const Range& range,
                        const Searcher& searcher);

} // namespace mtfind