#ifndef RDF4CPP_JSONLDPARSER_HPP
#define RDF4CPP_JSONLDPARSER_HPP

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

#include <rdf4cpp/parser/JsonLdParserTypes.hpp>

#include <generator>
#include <vector>
#include <forward_list>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace params {
        struct ParseContextTermParams {
            simdjson::ondemand::object local_context;
            json_ld::Context &active_context; // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::TermDefinition &term; // NOLINT(*-avoid-const-or-ref-data-members)
            std::vector<json_ld::TermDefinition> const &previous_terms; // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            bool is_protected = false;
            bool override_protected = false;
        };
        struct ParseContextIRIExpansionParams {
            json_ld::Context &active_context; // NOLINT(*-avoid-const-or-ref-data-members)
            simdjson::ondemand::object local_context;
            std::vector<json_ld::TermDefinition> const &previous_terms; // NOLINT(*-avoid-const-or-ref-data-members)
        };
        struct ParseParams {
            simdjson::ondemand::value element;
            json_ld::Context const &active_ctx; // NOLINT(*-avoid-const-or-ref-data-members)
            std::string_view base_iri;
            json_ld::IRIMapping const &active_graph; // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_subject; // NOLINT(*-avoid-const-or-ref-data-members)
            json_ld::IRIMapping const &active_property; // NOLINT(*-avoid-const-or-ref-data-members)
            std::variant<std::monostate, json_ld::IRIMapping, Literal> *obj_out = nullptr;
            bool is_top_level = false;
            bool is_reverse = false;
            bool is_json_literal = false;
            bool is_included = false;
        };
    }

    struct IStreamQuadIterator::ImplJsonLd final : Impl {
    private:
        state_type *state_;
        bool state_is_owned_;
        std::string json_data_;
        uint64_t blank_node_index_ = 0;

        static constexpr bool any_of(std::string_view v, std::initializer_list<std::string_view> l) {
            for (auto const x : l) {
                if (v == x) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] error_type make_error(ParsingError::Type t, std::string msg) const;
        json_ld::IRIMapping make_new_bn();
        nonstd::expected<IRI, error_type> make_iri(std::string_view i);
        nonstd::expected<IRI, error_type> make_iri(json_ld::IRIMapping const & m);
        nonstd::expected<Node, error_type> make_bn_or_iri(json_ld::IRIMapping const & m);
        nonstd::expected<Literal, error_type> make_literal(json_ld::LiteralMapping const & t);
        nonstd::expected<Literal, error_type> make_literal(json_ld::TypedLiteralMapping const & t);
        nonstd::expected<ok_type, error_type> make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, json_ld::IRIMapping const &object);
        nonstd::expected<ok_type, error_type> make_quad(json_ld::IRIMapping const &graph, json_ld::IRIMapping const &subject, json_ld::IRIMapping const &predicate, Node object);

        nonstd::expected<json_ld::Context, error_type> parse_context(simdjson::ondemand::value local_context,
                                                                     json_ld::Context const &active_context,
                                                                     std::string_view base_iri,
                                                                     bool override_protected = false,
                                                                     bool propagate = true);
        std::optional<error_type> parse_context_term(params::ParseContextTermParams p);

        nonstd::expected<json_ld::Context, error_type> parse_local_context(simdjson::padded_string_view json,
                                                                     json_ld::Context const &active_context,
                                                                     std::string_view base_iri,
                                                                     bool override_protected = false,
                                                                     bool propagate = true);


        nonstd::expected<json_ld::IRIMapping, error_type> iri_expansion(json_ld::Context const &active_context,
                                                                        std::optional<std::string_view> value,
                                                                        bool document_relative,
                                                                        bool vocab,
                                                                        json_ld::TermDefinition const *ignore_local = nullptr,
                                                                        params::ParseContextIRIExpansionParams* parse_ctx = nullptr);

        nonstd::expected<json_ld::ExpandedValue, error_type> value_expansion(json_ld::Context const &active_conext,
                                                                             json_ld::IRIMapping const &active_property,
                                                                             simdjson::ondemand::value value);
        nonstd::expected<json_ld::ExpandedValue, error_type> value_expansion(json_ld::Context const &active_conext,
                                                                             json_ld::IRIMapping const &active_property,
                                                                             std::string_view value);
        static json_ld::StringifyResult stringify(simdjson::ondemand::value v, bool normalize, bool force_double = false, bool escape_string = false);
        static json_ld::TypedLiteralMapping to_json_literal(simdjson::ondemand::value v);

        nonstd::expected<json_ld::ExpandedLevel, error_type> expand_level(json_ld::Context const &active_context,
                                                                          json_ld::IRIMapping const &active_property,
                                                                          simdjson::ondemand::value element,
                                                                          std::string_view base_iri,
                                                                          bool frame_expansion = false,
                                                                          bool ordered = false,
                                                                          bool from_map = false,
                                                                          bool assume_no_scalar = false,
                                                                          bool is_json_literal = false,
                                                                          std::optional<simdjson::ondemand::object> element_obj = std::nullopt);
        std::optional<error_type> expand_level_nested_recursive(json_ld::ExpandedMap& result,
                                                                simdjson::ondemand::object elem_obj,
                                                                json_ld::Context const & active_ctx,
                                                                json_ld::Context const & type_scoped_context,
                                                                json_ld::KeyPath const &active_path,
                                                                json_ld::IRIMapping const &active_property,
                                                                std::optional<std::string> const & input_type,
                                                                std::string_view base_iri,
                                                                bool reverse,
                                                                bool value_is_error);
        bool is_list_object(simdjson::ondemand::value v, json_ld::Context const & active_context);
        bool is_graph_object(simdjson::ondemand::value v, json_ld::Context const & active_context, std::optional<simdjson::ondemand::object>& obj_out);

        using result_generator = std::generator<nonstd::expected<ok_type, error_type>>;

        result_generator parse(params::ParseParams& p);
        result_generator parse(params::ParseParams& p, json_ld::ExpandedLevel &expanded);
        result_generator parse_list(simdjson::ondemand::value ar,
                               json_ld::Context const &active_ctx,
                               std::string_view base_iri,
                               json_ld::IRIMapping const &active_graph,
                               json_ld::IRIMapping const &active_subject,
                               json_ld::IRIMapping const &active_property,
                               std::variant<std::monostate, json_ld::IRIMapping, Literal>* obj_out,
                               bool recursive_list);

        result_generator parse();

        // if passed in value is an array, iterates over its content
        // otherwise iterates over [value]
        struct ValueArrayIter {
        private:
            simdjson::ondemand::array a_{};
            std::variant<std::monostate, simdjson::ondemand::value, simdjson::ondemand::array_iterator> current_;
            size_t current_index_ = 0;
            simdjson::ondemand::value cache;

        public:
            explicit ValueArrayIter(simdjson::ondemand::value v);
            simdjson::ondemand::value& operator*();
            simdjson::ondemand::value* operator->();
            ValueArrayIter& operator++();
            bool operator==(std::default_sentinel_t);
            ValueArrayIter& begin();
            std::default_sentinel_t end();
            void push_index(json_ld::KeyPath& p);
        };

        result_generator active_generator_;
        std::ranges::iterator_t<result_generator> current_iter_;

    public:
        [[nodiscard]] std::optional<nonstd::expected<ok_type, error_type>> next() override;
        [[nodiscard]] uint64_t current_line() const noexcept override;
        [[nodiscard]] uint64_t current_column() const noexcept override;

        explicit ImplJsonLd(std::string json, state_type *initial_state = nullptr);
        ImplJsonLd(void *stream,
                   ReadFunc read,
                   ErrorFunc error,
                   EOFFunc eof,
                   state_type *initial_state = nullptr);

        ~ImplJsonLd() override;
    };
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDPARSER_HPP
