#ifndef RDF4CPP_JSONLDCONTEXTPARSER_HPP
#define RDF4CPP_JSONLDCONTEXTPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>

#include <rdf4cpp/parser/JsonLdParserTypes.hpp>

#include <vector>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace params {
        struct ParseContextTermParams {
            simdjson::ondemand::object local_context;
            json_ld::Context &active_context;                            // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::TermDefinition &term;                               // NOLINT(*-avoid-const-or-ref-data-members)
            std::vector<json_ld::TermDefinition> const &previous_terms;  // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            bool is_protected = false;
            bool override_protected = false;
        };
        struct ParseContextIRIExpansionParams {
            json_ld::Context &active_context;  // NOLINT(*-avoid-const-or-ref-data-members)
            simdjson::ondemand::object local_context;
            std::vector<json_ld::TermDefinition> const &previous_terms;  // NOLINT(*-avoid-const-or-ref-data-members)
        };
    }  // namespace params

    namespace json_ld {
        struct ContextParser {
            using error_type = ParsingError;
            IRIFactory *iri_factory;

            nonstd::expected<Context, error_type> parse_context(simdjson::ondemand::value local_context,
                                                                Context const &active_context,
                                                                std::string_view base_iri,
                                                                bool override_protected = false,
                                                                bool propagate = true);
            std::optional<error_type> parse_context_term(params::ParseContextTermParams p);

            nonstd::expected<Context, error_type> parse_local_context(simdjson::padded_string_view json,
                                                                      Context const &active_context,
                                                                      std::string_view base_iri,
                                                                      bool override_protected = false,
                                                                      bool propagate = true);


            nonstd::expected<IRIMapping, error_type> iri_expansion(Context const &active_context,
                                                                   std::optional<std::string_view> value,
                                                                   bool document_relative,
                                                                   bool vocab,
                                                                   TermDefinition const *ignore_local = nullptr,
                                                                   params::ParseContextIRIExpansionParams *parse_ctx = nullptr);
        };
    }  // namespace json_ld
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDCONTEXTPARSER_HPP
