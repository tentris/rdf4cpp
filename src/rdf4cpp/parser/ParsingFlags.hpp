#ifndef RDF4CPP_PARSER_PARSINGFLAGS_HPP
#define RDF4CPP_PARSER_PARSINGFLAGS_HPP

#include <cstdint>
#include <type_traits>

namespace rdf4cpp::parser {

/**
 * Note that the syntax flags are mutually exclusive.
 * If none is used, Turtle is the default.
 * If more than one is used accidentally at the same time, TriG is likely the result (even if it does never get specified).
 *
 * Not every flag applies to every syntax. A flag that the parser of the selected syntax does not
 * know is ignored without a diagnostic:
 * - Lax and NoParsePrefix are only used by Turtle, TriG, N-Triples and N-Quads.
 * - KeepBlankNodeIds and NoParseBlankNode are used by Turtle, TriG, N-Triples, N-Quads and JSON-LD.
 * - the JsonLdDirection flags are only used by JSON-LD.
 * - RDF/XML uses none of them.
 */
enum struct ParsingFlag : uint16_t {
    /**
     * Continue after recoverable syntax errors instead of stopping at the first one.
     */
    Lax              = 1 << 0,
    /**
     * Reject prefix definitions in the document.
     */
    NoParsePrefix    = 1 << 1,
    /**
     * Use the blank node labels of the document as they are, instead of mapping them through the
     * blank node scope manager of the ParsingState.
     * @note for JSON-LD this can make a document label name a blank node that the parser generated
     * for a node object without @id.
     */
    KeepBlankNodeIds = 1 << 2,
    /**
     * Report a parsing error for every blank node.
     * @note JSON-LD also generates blank nodes for node objects without @id, those are reported too.
     */
    NoParseBlankNode = 1 << 3,

    Turtle   = 0b00 << 4, // default
    NTriples = 0b01 << 4,
    NQuads   = 0b10 << 4,
    TriG     = 0b11 << 4,
    RdfXml  = 0b100 << 4,
    JsonLd  = 0b101 << 4,

    /**
     * Drop the base direction of JSON-LD string values.
     */
    JsonLdDirectionNone = 0b00 << 7,
    /**
     * Encode the base direction of JSON-LD string values in a datatype of the
     * https://www.w3.org/ns/i18n# namespace.
     */
    JsonLdDirectionI18n = 0b01 << 7,
    /**
     * Encode the base direction of JSON-LD string values as a blank node with rdf:value,
     * rdf:language and rdf:direction.
     */
    JsonLdDirectionCompound = 0b10 << 7,
};
constexpr uint16_t ParsingFlag_SyntaxMask = 0b111 << 4;
constexpr uint16_t ParsingFlag_JsonLdDirectionMask = 0b11 << 7;

struct ParsingFlags {
private:
    using flag_u_type = std::underlying_type_t<ParsingFlag>;
    flag_u_type flags;

    constexpr ParsingFlags(uint16_t const flags) noexcept : flags{flags} {}

public:
    constexpr ParsingFlags(ParsingFlag const flag) noexcept
        : flags{static_cast<flag_u_type>(flag)} {
    }

    static constexpr ParsingFlags none() noexcept {
        return ParsingFlags{0};
    }

    [[nodiscard]] constexpr bool contains(ParsingFlag const flag) const noexcept {
        return flags & static_cast<flag_u_type>(flag);
    }

    constexpr ParsingFlags &operator|=(ParsingFlags const other) noexcept {
        this->flags |= other.flags;
        return *this;
    }

    constexpr ParsingFlags operator|(ParsingFlags const other) const noexcept {
        auto cpy = *this;
        return (cpy |= other);
    }

    constexpr ParsingFlags &operator|=(ParsingFlag const flag) noexcept {
        this->flags |= static_cast<flag_u_type>(flag);
        return *this;
    }

    constexpr ParsingFlags operator|(ParsingFlag const flag) const noexcept {
        auto cpy = *this;
        return (cpy |= flag);
    }

    /**
     * @return the syntax ParsingFlag contained in this ParsingFlags. (Turtle if not specified)
     */
    [[nodiscard]] constexpr ParsingFlag get_syntax() const noexcept {
        return static_cast<ParsingFlag>(flags & static_cast<flag_u_type>(ParsingFlag_SyntaxMask));
    }

    /**
     * @return the JsonLD direction ParsingFlag contained in this ParsingFlags. (None if not specified)
     */
    [[nodiscard]] constexpr ParsingFlag get_direction() const noexcept {
        return static_cast<ParsingFlag>(flags & static_cast<flag_u_type>(ParsingFlag_JsonLdDirectionMask));
    }

    [[nodiscard]] constexpr bool syntax_allows_prefixes() const noexcept {
        auto const syn = get_syntax();
        return syn == ParsingFlag::Turtle || syn ==  ParsingFlag::TriG;
    }
};

constexpr ParsingFlags operator|(ParsingFlag const f1, ParsingFlag const f2) noexcept {
    return ParsingFlags::none() | f1 | f2;
}

} // namespace rdf4cpp::parser

#endif  //RDF4CPP_PARSER_PARSINGFLAGS_HPP
