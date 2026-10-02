/**
 * @file searcher.hpp
 * @brief Поиск вхождений маски в строке.
 */

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace mtfind {

/**
 * @brief Одно найденное вхождение в строке.
 */
struct Match {
    std::size_t position = 0; ///< Позиция начала вхождения в строке (0-based).
    std::string text;         ///< Найденная подстрока.
};

/**
 * @brief Ищет непересекающиеся вхождения маски в строке.
 *
 * Для точного поиска используется алгоритм Boyer–Moore–Horspool (BMH):
 * он строит таблицу сдвигов по символам маски и позволяет пропускать
 * большие участки текста без сравнения.
 */
class Searcher {
public:
    /**
     * @brief Создаёт поисковик для заданной маски.
     *
     * @param mask Маска поиска.
     * @throw std::invalid_argument если маска пуста.
     */
    explicit Searcher(std::string mask);

    /**
     * @brief Находит все непересекающиеся вхождения в строке.
     *
     * Порядок вхождений совпадает с их порядком в строке.
     *
     * @param line Строка, в которой ищутся вхождения (без символа конца строки).
     * @return Список найденных вхождений.
     */
    [[nodiscard]] std::vector<Match> find_all(const std::string& line) const;

    /// @return Длина маски в символах.
    [[nodiscard]] std::size_t mask_size() const noexcept { return mask_.size(); }

    /// @return Маска в исходном виде.
    [[nodiscard]] const std::string& mask() const noexcept { return mask_; }

private:
    std::string mask_;                     ///< Исходная маска.
    std::array<std::size_t, 256> shift_{}; ///< Таблица сдвигов BMH по байтам.

    /**
     * @brief Заполняет таблицу сдвигов для BMH.
     */
    void build_shift_table();
};

} // namespace mtfind