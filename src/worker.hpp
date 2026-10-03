/**
 * @file worker.hpp
 * @brief Обработка одного байтового диапазона файла.
 *
 * Воркер открывает файл, читает строки в диапазоне [start, end), ищет
 * вхождения маски и пишет их во временный файл. Глобальные номера строк
 * восстанавливаются на этапе слияния, когда известно, сколько строк
 * содержит каждый предыдущий диапазон.
 */

#pragma once

#include "file_splitter.hpp"
#include "searcher.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace mtfind {

/**
 * @brief Результат работы воркера.
 */
struct WorkerResult {
    std::filesystem::path temp_path; ///< Путь к временному файлу с вхождениями.
    std::size_t line_count = 0;      ///< Сколько строк обработано в диапазоне.
    std::size_t match_count = 0;     ///< Сколько вхождений найдено.
};

/**
 * @brief Обрабатывает один диапазон файла.
 *
 * Формат временного файла — построчно:
 * @code
 * <relative_line> <position> <text>\n
 * @endcode
 * где @c relative_line — номер строки внутри диапазона (1-based),
 * @c position — позиция вхождения в строке (0-based), @c text — подстрока.
 * Текст может содержать пробелы; разделителем служат первые два пробела.
 *
 * @param filename Путь к исходному файлу.
 * @param range Диапазон байт [start, end). Должен начинаться с начала строки.
 * @param searcher Поисковик с уже скомпилированной маской.
 * @param temp_dir Директория для временного файла.
 * @return Статистика воркера и путь к его временному файлу.
 * @throw std::runtime_error если файл не открывается или temp-файл не создаётся.
 */
WorkerResult run_worker(const std::string& filename,
                        const Range& range,
                        const Searcher& searcher,
                        const std::filesystem::path& temp_dir);

} // namespace mtfind