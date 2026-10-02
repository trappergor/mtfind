/**
 * @file searcher.hpp
 * @brief Поиск вхождений маски в строке.
 *
 * Поддерживается маска с символом '?', обозначающим любой одиночный символ.
 * Реализация использует предварительный поиск якорного сегмента алгоритмом
 * Boyer–Moore–Horspool и последующую проверку остальных сегментов.
 */

#pragma once

#include "mask.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace mtfind {

/// Значение "позиция не найдена".
inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

/**
 * @brief Одно найденное вхождение в строке.
 */
struct Match {
    std::size_t position = 0; ///< Позиция начала вхождения в строке (0-based).
    std::string text;         ///< Фактически найденная подстрока (с учётом '?').
};

/**
 * @brief Матчер одной подстроки алгоритмом Boyer–Moore–Horspool.
 *
 * Используется как "движок" для поиска якорного сегмента. Хранит иглу
 * и таблицу сдвигов, метод search() возвращает первое вхождение иглы
 * в haystack на позиции не меньше from или npos.
 */
class BmhMatcher {
public:
    /// Создаёт пустой матчер (search всегда возвращает npos).
    BmhMatcher() = default;

    /**
     * @brief Создаёт матчер для заданной иглы.
     * @param needle Игла. Пустая игла допустима — search будет возвращать npos.
     */
    explicit BmhMatcher(std::string needle);

    /**
     * @brief Ищет первое вхождение иглы в haystack начиная с позиции @p from.
     * @return Позиция начала вхождения или npos.
     */
    [[nodiscard]] std::size_t search(const std::string& haystack,
                                     std::size_t from) const noexcept;

    /// @return Игла.
    [[nodiscard]] const std::string& needle() const noexcept { return needle_; }

    /// @return true, если игла пуста.
    [[nodiscard]] bool empty() const noexcept { return needle_.empty(); }

private:
    std::string needle_;                   ///< Игла.
    std::array<std::size_t, 256> shift_{}; ///< Таблица сдвигов BMH.
};

/**
 * @brief Ищет непересекающиеся вхождения маски в строке.
 *
 * Алгоритм:
 *  1. Маска компилируется один раз (compile_mask).
 *  2. В строке ищется якорный сегмент алгоритмом BMH.
 *  3. Для каждого найденного якоря проверяются остальные сегменты.
 *  4. После успешного вхождения поиск продолжается не раньше его конца —
 *     вхождения не пересекаются.
 *  5. Вырожденные случаи (все '?', нет '?') обрабатываются отдельными
 *     короткими путями.
 */
class Searcher {
public:
    /**
     * @brief Создаёт поисковик для заданной маски.
     * @param mask Маска поиска, может содержать '?'.
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
    [[nodiscard]] std::size_t mask_size() const noexcept { return compiled_.length; }

    /// @return Исходная маска.
    [[nodiscard]] const std::string& mask() const noexcept { return compiled_.mask; }

private:
    /// Путь для маски, состоящей только из '?'.
    [[nodiscard]] std::vector<Match> find_all_wildcards_only(
        const std::string& line) const;

    /// Путь для точной маски (без '?').
    [[nodiscard]] std::vector<Match> find_all_exact(
        const std::string& line) const;

    /// Путь для смешанной маски: поиск по якорю + проверка сегментов.
    [[nodiscard]] std::vector<Match> find_all_with_anchor(
        const std::string& line) const;

    /// Проверяет все сегменты маски в позиции @p candidate.
    [[nodiscard]] bool verify_at(const std::string& line,
                                 std::size_t candidate) const noexcept;

    CompiledMask compiled_;      ///< Скомпилированная маска.
    BmhMatcher anchor_matcher_;  ///< Матчер якорного сегмента (пуст при no_anchor).
};

} // namespace mtfind