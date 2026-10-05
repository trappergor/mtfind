/**
 * @file worker.hpp
 * @brief Обработка одного байтового диапазона файла.
 * 
 * Воркер читает строки в диапазоне [start, end), ищет вхождения маски
 * и пишет их во временный файл. Глобальные номера строк восстанавливаются
 * на этапе слияния, когда известно, сколько строк содержит каждый
 * предыдущий диапазон.
 *
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
 * @brief Генерирует уникальное имя временного файла в заданной директории.
 *
 * Использует атомарный счётчик и таймстемп
 *
 * @param dir Каталог для временного файла.
 * @return Полный путь к файлу (файл не создаётся).
 */
std::filesystem::path make_temp_path(const std::filesystem::path& dir);

/**
 * @brief Обрабатывает один диапазон файла.
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