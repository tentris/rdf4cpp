#ifndef XMLPARSERCOLLECTIONSTATE_HPP
#define XMLPARSERCOLLECTIONSTATE_HPP

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>

namespace rdf4cpp::parser {
    /**
     * state for https://www.w3.org/TR/rdf11-xml/#parseTypeCollectionPropertyElt
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::CollectionState final : BaseState {
        void on_characters(ImplXML &i, std::string_view chars) override;
        void on_start_element(ImplXML &i, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) override;
        void on_end_element(ImplXML &i) override;
        void move_to(BaseState *b) noexcept override;

        Node subject;
        IRI predicate;
        Node last_bn = Node::make_null();
        IRI reify;
        bool first = true;

        CollectionState(InheritedAttributeInfo const &i, Node sub, IRI pred, IRI reify)
            : BaseState(i), subject(sub), predicate(pred), reify(reify) {
        }

        static constexpr std::string_view iri_nil = "http://www.w3.org/1999/02/22-rdf-syntax-ns#nil";
        static constexpr std::string_view iri_rest = "http://www.w3.org/1999/02/22-rdf-syntax-ns#rest";
        static constexpr std::string_view iri_first = "http://www.w3.org/1999/02/22-rdf-syntax-ns#first";
    };
}  // namespace rdf4cpp::parser

#endif  // XMLPARSERCOLLECTIONSTATE_HPP