#ifndef RDF4CPP_XMLPARSERBASESTATE_H
#define RDF4CPP_XMLPARSERBASESTATE_H

#include <rdf4cpp/parser/XMLParserStateCollector.hpp>

namespace rdf4cpp::parser {
    /**
     * most states handle one or more of the elements in https://www.w3.org/TR/rdf11-xml/#section-Infoset-Grammar .
     * note that the creation of a state is done by on_start_element of the previous state.
     * each state holds information on base iri and language tag defined on the corresponding xml element.
     */
    struct IStreamQuadIterator::ImplXMLStateCollector::BaseState {  // NOLINT(*-special-member-functions)
        virtual ~BaseState() = default;
        virtual void on_characters(ImplXML &impl, std::string_view chars) = 0;
        virtual void on_start_element(ImplXML &impl, std::string_view local_name, std::string_view uri, std::span<Attribute> attributes) = 0;
        virtual void on_end_element(ImplXML &impl) = 0;
        virtual void move_to(BaseState *b) noexcept = 0;

        struct InheritedAttributeInfo {
            std::string_view base = "";
            std::string_view lang_tag = "";
        };

        std::string base;
        std::string lang_tag;

        explicit BaseState(InheritedAttributeInfo const &i)
            : base(i.base), lang_tag(i.lang_tag) {
        }

        static constexpr std::string_view base_attribute = "http://www.w3.org/XML/1998/namespacebase";
        static constexpr std::string_view lang_attribute = "http://www.w3.org/XML/1998/namespacelang";
        static InheritedAttributeInfo get_inherited_attributes(ImplXML &impl, std::span<Attribute> attributes);
    };
}

#endif  //RDF4CPP_XMLPARSERBASESTATE_H
