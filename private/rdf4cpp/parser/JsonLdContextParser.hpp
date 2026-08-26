#ifndef RDF4CPP_JSONLDCONTEXTPARSER_HPP
#define RDF4CPP_JSONLDCONTEXTPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

#include <rdf4cpp/parser/JsonLdParserTypes.hpp>

#include <vector>
#include <map>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace params {
        struct ParseContextParams {
            json_ld::Context const &active_context; // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            std::string_view base_url;
            bool override_protected = false;
            bool propagate = true;
            bool validate_scoped_contexts = true;
        };
        struct ParseContextTermParams {
            simdjson::ondemand::object local_context;
            json_ld::Context &active_context;                            // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::TermDefinition &term;                               // NOLINT(*-avoid-const-or-ref-data-members)
            std::vector<json_ld::TermDefinition> const &previous_terms;  // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            std::string_view base_url;
            bool is_protected = false;
            bool override_protected = false;
            bool validate_scoped_contexts = true;
        };
        struct ParseContextIRIExpansionParams {
            json_ld::Context &active_context;  // NOLINT(*-avoid-const-or-ref-data-members)
            simdjson::ondemand::object local_context;
            std::vector<json_ld::TermDefinition> const &previous_terms;  // NOLINT(*-avoid-const-or-ref-data-members)
        };
    }  // namespace params

    namespace json_ld {
        struct RemoteContextCache {
            std::map<std::string, RemoteContextEntry, std::less<>> contexts;

            [[nodiscard]] size_t num_active_entries() const noexcept;
            std::pair<RemoteContextEntry&, bool> get(std::string_view url);
            RemoteContextEntry* try_get(std::string_view url);
        };

        struct ContextParser {
            using error_type = ParsingError;
            IStreamQuadIterator::state_type* parse_state;
            std::string original_base_iri;
            RemoteContextCache remote_contexts;
            /**
             * If set, blank node labels of the document are used as they are. Otherwise they get
             * document_bnode_prefix, which keeps them apart from the labels the parser generates.
             */
            bool keep_document_bnode_labels;

            static constexpr size_t remote_context_size_limit = 100;

            inline explicit ContextParser(std::string base_iri, bool const keep_document_bnode_labels, IStreamQuadIterator::state_type *parse_state)
                : parse_state(parse_state),
                  original_base_iri(std::move(base_iri)),
                  keep_document_bnode_labels(keep_document_bnode_labels) {
            }

            /**
             * Sets the base of iri_factory, skipping the validation if it is already set to base.
             */
            void set_resolution_base(std::string_view base);

            nonstd::expected<Context, error_type> parse_context(simdjson::ondemand::value local_context, params::ParseContextParams p);
            std::optional<error_type> parse_context_term(params::ParseContextTermParams p);

            nonstd::expected<Context, error_type> parse_local_context(simdjson::padded_string_view json, params::ParseContextParams p, bool skip_to_context = false);


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
