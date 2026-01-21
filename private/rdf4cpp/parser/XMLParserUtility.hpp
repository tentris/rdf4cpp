#ifndef RDF4CPP_XMLPARSERSTATECOLLECTOR_H
#define RDF4CPP_XMLPARSERSTATECOLLECTOR_H

#include <deque>

#include <rdf4cpp/Expected.hpp>

#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>
#include <rdf4cpp/parser/ParsingError.hpp>
#include <rdf4cpp/parser/ParsingState.hpp>

#include <libxml/parser.h>

#include <dice/sparse-map/sparse_set.hpp>

namespace rdf4cpp::parser {
    inline std::string_view from_xml_char(xmlChar const *s) {
        if (s == nullptr) {
            return "";
        }
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return {reinterpret_cast<char const *>(s)};
    }

    inline std::string_view from_xml_char(xmlChar const *s, xmlChar const *e) {
        if (s == nullptr) {
            return "";
        }
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return {reinterpret_cast<char const *>(s), reinterpret_cast<char const *>(e)};
    }

    inline std::string_view from_xml_char(xmlChar const *s, int const n) {
        if (s == nullptr) {
            return "";
        }
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return {reinterpret_cast<char const *>(s), static_cast<size_t>(n)};
    }

    struct XMLAttribute {
        xmlChar const *local_name_raw;
        xmlChar const *prefix_raw;
        xmlChar const *uri_raw;
        xmlChar const *value_start_raw;
        xmlChar const *value_end_raw;

        [[nodiscard]] std::string_view value() const {
            return from_xml_char(value_start_raw, value_end_raw);
        }

        [[nodiscard]] std::string_view local_name() const {
            return from_xml_char(local_name_raw);
        }

        [[nodiscard]] std::string_view uri() const {
            return from_xml_char(uri_raw);
        }
    };

    struct XMLStateInfo {
        uint64_t line;
        uint64_t column;
        std::string_view base;
        std::string_view lang_tag;
        std::string_view source;
        int source_offset;
    };

    struct XMLOutputQueue {
        using value_type = IStreamQuadIterator::value_type;
        using state_type = IStreamQuadIterator::state_type;

    private:
        std::deque<value_type> result_queue_;
        size_t next_bn_index_ = 0;
        state_type *state_;
        bool state_is_owned_ = false;
        dice::sparse_map::sparse_set<storage::identifier::NodeBackendID> reserved_ids_;

        static constexpr std::string_view reify_subject = "http://www.w3.org/1999/02/22-rdf-syntax-ns#subject";
        static constexpr std::string_view reify_predicate = "http://www.w3.org/1999/02/22-rdf-syntax-ns#predicate";
        static constexpr std::string_view reify_object = "http://www.w3.org/1999/02/22-rdf-syntax-ns#object";
        static constexpr std::string_view reify_type = "http://www.w3.org/1999/02/22-rdf-syntax-ns#Statement";

        template<typename NT>
        [[nodiscard]] NT inspect_node(NT node, XMLStateInfo const &i);

    public:
        explicit XMLOutputQueue(state_type *state);
        ~XMLOutputQueue();

        XMLOutputQueue(XMLOutputQueue const &) = delete;
        XMLOutputQueue &operator=(XMLOutputQueue const &) = delete;
        XMLOutputQueue(XMLOutputQueue &&) = delete;
        XMLOutputQueue &operator=(XMLOutputQueue &&) = delete;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::optional<value_type> next();
        [[nodiscard]] std::string_view current_base_iri() const;

        void add_error(ParsingError::Type ty, std::string msg, XMLStateInfo const &i);
        void add_old_term_error(XMLStateInfo const &i);
        /**
         * add statement to the output list, if none of the components is null
         * (null is used to track an already inserted parse error for that component)
         */
        void add_statement(Node subject, IRI predicate, Node object, IRI reify);
        /**
         * create an IRI with no checks, intended for hardcoded IRIs like reify_subject
         */
        [[nodiscard]] IRI make_hardcoded_iri(std::string_view iri) const;
        [[nodiscard]] IRI make_type_iri() const;
        [[nodiscard]] IRI make_iri(std::string_view iri, std::string_view base, XMLStateInfo const &i);
        [[nodiscard]] IRI make_iri(std::string_view uri, std::string_view local_name, std::string_view base, XMLStateInfo const &i);
        /**
         * create the IRI for an id_attrib, including uniqueness check
         */
        [[nodiscard]] IRI make_id(std::string_view local_name, std::string_view base, XMLStateInfo const &i);
        [[nodiscard]] Node make_bn(std::optional<std::string_view> name, XMLStateInfo const &i);
        /**
         * creates a literal
         * @param value
         * @param datatype
         * @param lang_tag (ignored, if datatype is set)
         * @param i
         * @return
         */
        [[nodiscard]] Literal make_literal(std::string_view value, std::optional<IRI> datatype, std::optional<std::string_view> lang_tag, XMLStateInfo const &i);
    };

    struct PopState {};
    struct NoStateChange {};

    struct StateTransition;

    /**
     * removes whitespace according to xml spec
     */
    [[nodiscard]] std::string_view trim_left(std::string_view v);
    [[nodiscard]] bool iri_equal_pieces(std::string_view full_iri, std::string_view uri, std::string_view local_name);
    [[nodiscard]] bool iri_reserved(std::string_view uri, std::string_view local_name);
    [[nodiscard]] bool iri_core_syntax(std::string_view uri, std::string_view local_name);
    [[nodiscard]] bool iri_old_term(std::string_view uri, std::string_view local_name);
    [[nodiscard]] bool iri_in_xml_namespace(std::string_view uri, std::string_view local_name);
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_XMLPARSERSTATECOLLECTOR_H
