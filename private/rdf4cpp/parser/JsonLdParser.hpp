#ifndef RDF4CPP_JSONLDPARSER_HPP
#define RDF4CPP_JSONLDPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

#include <rdf4cpp/parser/JsonLdExpandParser.hpp>
#include <rdf4cpp/parser/JsonLdParserTypes.hpp>

#include <generator>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace json_ld {
        struct DirectionLiteralResult {
            Node object;
            std::optional<std::array<Quad, 3>> extra_quads = std::nullopt;
        };
    }  // namespace json_ld
    namespace params {
        // result of this parsing step for use in lists
        // Node => parsed literal (might be a bn in case of compound literals)
        // IRIMapping => IRI or bn
        // monostate => nothing (empty, or not mapped to json-ld triples)
        using ListObjOut = std::variant<std::monostate, json_ld::IRIMapping, Node>;

        struct ParseParams {
            simdjson::ondemand::value element;
            json_ld::Context const &active_ctx;  // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            json_ld::IRIMapping const &active_graph;     // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_subject;   // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_property;  // NOLINT(*-avoid-const-or-ref-data-members)
            ListObjOut *obj_out = nullptr;
            bool is_top_level = false;
            bool is_reverse = false;
            bool is_json_literal = false;
            bool is_included = false;
        };
        struct ParseListParams {
            simdjson::ondemand::value ar;
            json_ld::Context const &active_ctx;  // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            json_ld::IRIMapping const &active_graph;     // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_subject;   // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_property;  // NOLINT(*-avoid-const-or-ref-data-members)
            std::variant<std::monostate, json_ld::IRIMapping, Node> *obj_out;
            bool recursive_list;
        };
    }  // namespace params

    struct IStreamQuadIterator::ImplJsonLd final : Impl {
    private:
        state_type *state_;
        bool state_is_owned_;
        std::string json_data_;
        uint64_t blank_node_index_ = 0;
        json_ld::ExpandParser expand_parser_;
        ParsingFlag direction_;

        json_ld::IRIMapping make_new_bn();
        nonstd::expected<IRI, error_type> make_iri(std::string_view iri);
        nonstd::expected<IRI, error_type> make_iri(json_ld::IRIMapping const &iri);
        nonstd::expected<Node, error_type> make_bn_or_iri(json_ld::IRIMapping const &mapping);
        nonstd::expected<json_ld::DirectionLiteralResult, error_type> make_literal(json_ld::StringLikeLiteralMapping const &lit, json_ld::IRIMapping const &graph);
        nonstd::expected<Literal, error_type> make_literal(json_ld::TypedLiteralMapping const &lit);
        nonstd::expected<ok_type, error_type> make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, json_ld::IRIMapping const &object);
        nonstd::expected<ok_type, error_type> make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, Node object);

        using result_generator = std::generator<nonstd::expected<ok_type, error_type>>;

        result_generator parse(params::ParseParams p);
        result_generator parse(params::ParseParams p, json_ld::ExpandedLevel &expanded);
        result_generator parse_list(params::ParseListParams p);

        result_generator parse();

        result_generator active_generator_;
        std::ranges::iterator_t<result_generator> current_iter_;

        static constexpr size_t BufferSizeMult = 5;

    public:
        [[nodiscard]] std::optional<nonstd::expected<ok_type, error_type>> next() override;
        [[nodiscard]] uint64_t current_line() const noexcept override;
        [[nodiscard]] uint64_t current_column() const noexcept override;

        explicit ImplJsonLd(std::string json, ParsingFlags flags, state_type *initial_state = nullptr);
        ImplJsonLd(void *stream,
                   ReadFunc read,
                   ErrorFunc error,
                   EOFFunc eof,
                   ParsingFlags flags,
                   state_type *initial_state = nullptr);

        ~ImplJsonLd() override;
    };
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDPARSER_HPP
