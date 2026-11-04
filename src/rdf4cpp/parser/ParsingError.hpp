#ifndef RDF4CPP_PARSER_PARSINGERROR_HPP
#define RDF4CPP_PARSER_PARSINGERROR_HPP

#include <cstdint>
#include <ostream>
#include <string>

namespace rdf4cpp::parser {

/**
 * An error that might occur while parsing a turtle file
 */
struct ParsingError {
    enum struct Type : uint8_t {
        Internal = 0,
        EofReached,
        BadSyntax,
        BadIri,
        BadCurie,
        BadLiteral,
        BadBlankNode,
        /**
         * JSON-LD: a remote context or `@import` could not be loaded (see ParsingState::request_url),
         * or the loaded document is no JSON-LD context.
         */
        RemoteContext,
        /**
         * JSON-LD: a limit of ParsingState was reached (stack_space_limit, remote_context_size_limit).
         */
        LimitExceeded,
    };

    Type error_type;
    uint64_t line;
    uint64_t col;
    std::string message;
};

} // namespace rdf4cpp::parser

namespace std {

template<>
struct formatter<::rdf4cpp::parser::ParsingError::Type> : formatter<string_view> {
    template<typename FmtCtx>
    auto format(::rdf4cpp::parser::ParsingError::Type const &err, FmtCtx &ctx) const {
        using ::rdf4cpp::parser::ParsingError;
        switch (err) {
            case ParsingError::Type::Internal:
                return format_to(ctx.out(), "internal error");
            case ParsingError::Type::EofReached:
                return format_to(ctx.out(), "unexpected end of file");
            case ParsingError::Type::BadSyntax:
                return format_to(ctx.out(), "bad syntax");
            case ParsingError::Type::BadIri:
                return format_to(ctx.out(), "bad iri");
            case ParsingError::Type::BadCurie:
                return format_to(ctx.out(), "bad curie");
            case ParsingError::Type::BadLiteral:
                return format_to(ctx.out(), "bad literal");
            case ParsingError::Type::BadBlankNode:
                return format_to(ctx.out(), "bad blank node");
            case ParsingError::Type::RemoteContext:
                return format_to(ctx.out(), "remote context failed");
            case ParsingError::Type::LimitExceeded:
                return format_to(ctx.out(), "limit exceeded");
            default:
                return format_to(ctx.out(), "unknown");
        }
    }
};

template<>
struct formatter<::rdf4cpp::parser::ParsingError> : formatter<string_view> {
    template<typename FmtCtx>
    auto format(::rdf4cpp::parser::ParsingError const &err, FmtCtx &ctx) const {
        return format_to(ctx.out(), "{}:{}({}): {}", err.line, err.col, err.error_type, err.message);
    }
};

} // namespace std

namespace rdf4cpp::parser {

inline std::ostream &operator<<(std::ostream &os, ParsingError::Type const &error_type) {
    std::format_to(std::ostreambuf_iterator<char>{os}, "{}", error_type);
    return os;
}

inline std::ostream &operator<<(std::ostream &os, ParsingError const &error) {
    std::format_to(std::ostreambuf_iterator<char>{os}, "{}", error);
    return os;
}

} // namespace rdf4cpp::parser

#endif  //RDF4CPP_PARSER_PARSINGERROR_HPP
