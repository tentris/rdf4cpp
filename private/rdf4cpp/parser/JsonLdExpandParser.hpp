#ifndef RDF4CPP_JSONLDEXPANDPARSER_HPP
#define RDF4CPP_JSONLDEXPANDPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/parser/JsonLdContextParser.hpp>
#include <rdf4cpp/parser/JsonLdParserTypes.hpp>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace params {
        struct ExpandParams {
            json_ld::Context const &active_context;      // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_property;  // NOLINT(*-avoid-const-or-ref-data-members)
            simdjson::ondemand::value element;
            std::string_view base_iri;
            bool from_map = false;
            bool assume_no_scalar = false;
            bool is_json_literal = false;
            std::optional<simdjson::ondemand::object> element_obj = std::nullopt;
        };
        struct ExpandNestedParams {
            json_ld::ExpandedMap &result;  // NOLINT(*-avoid-const-or-ref-data-members)
            simdjson::ondemand::object elem_obj;
            json_ld::Context const &active_ctx;            // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::Context const &type_scoped_context;   // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::KeyPath const &active_path;           // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_property;    // NOLINT(*-avoid-const-or-ref-data-members)
            std::optional<std::string> const &input_type;  // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            bool reverse = false;
            bool value_is_error = false;
        };
    }  // namespace params

    namespace json_ld {
        struct ExpandParser {
            using error_type = ParsingError;
            ContextParser context_parser;

            inline explicit ExpandParser(IRIFactory &f, std::string base_iri)
                : context_parser(&f, std::move(base_iri)) {
            }

            nonstd::expected<ExpandedValue, error_type> value_expansion(Context const &active_conext,
                                                                        IRIMapping const &active_property,
                                                                        simdjson::ondemand::value value);
            nonstd::expected<ExpandedValue, error_type> value_expansion(Context const &active_conext,
                                                                        IRIMapping const &active_property,
                                                                        std::string_view value);
            static StringifyResult stringify(simdjson::ondemand::value v, bool normalize, bool force_double = false, bool escape_string = false);
            static TypedLiteralMapping to_json_literal(simdjson::ondemand::value v);

            nonstd::expected<ExpandedLevel, error_type> expand_level(params::ExpandParams p);
            std::optional<error_type> expand_level_nested_recursive(params::ExpandNestedParams p);
            bool is_list_object(simdjson::ondemand::value v, Context const &active_context);
            bool is_graph_object(simdjson::ondemand::value v, Context const &active_context, std::optional<simdjson::ondemand::object> &obj_out);
        };
    }  // namespace json_ld
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDEXPANDPARSER_HPP
