#ifndef RDF4CPP_PARSER_FORMATGUESS_HPP
#define RDF4CPP_PARSER_FORMATGUESS_HPP

#include <cstdint>
#include <string_view>

#include <rdf4cpp/parser/ParsingFlags.hpp>

namespace rdf4cpp::parser {

    /**
     * Confidence level for an RDF format guess, ordered from least to most confident.
     */
    enum struct GuessConfidence : uint8_t {
        None = 0,  ///< no guess could be made
        Low,       ///< weak heuristic match (e.g., ambiguous extension like .owl)
        Medium,    ///< content sniffing with good signal
        High,      ///< file extension match or strong content match
        Certain,   ///< unambiguous (extension + content agree)
    };

    /**
     * Result of an RDF serialization format guess, combining the detected syntax with
     * a confidence level indicating how reliable the guess is.
     */
    struct FormatGuess {
        ParsingFlag syntax = ParsingFlag::Auto;
        GuessConfidence confidence = GuessConfidence::None;

        [[nodiscard]] constexpr bool is_known() const noexcept {
            return syntax != ParsingFlag::Auto && confidence != GuessConfidence::None;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return is_known();
        }
    };

    /**
     * Guess the RDF serialization format from a file extension (including the dot).
     * Case-insensitive. Returns {Auto, None} for unrecognized extensions.
     */
    [[nodiscard]] FormatGuess guess_format_from_extension(std::string_view extension) noexcept;

    /**
     * Extract the file extension from a path and guess the format.
     */
    [[nodiscard]] FormatGuess guess_format_from_path(std::string_view file_path) noexcept;

    /**
     * Guess the RDF serialization format by inspecting a prefix of the file content.
     * At least 512 bytes recommended, 4096 bytes ideal.
     * Strips a leading UTF-8 BOM and skips whitespace and #-comment lines before sniffing.
     */
    [[nodiscard]] FormatGuess guess_format_from_content(std::string_view prefix) noexcept;

    /**
     * Combined guess: extension first, content second.
     * Confidence boosted to Certain when both agree.
     */
    [[nodiscard]] FormatGuess guess_format(std::string_view file_path, std::string_view prefix) noexcept;

}  // namespace rdf4cpp::parser

#endif  // RDF4CPP_PARSER_FORMATGUESS_HPP
