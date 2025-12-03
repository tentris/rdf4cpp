#ifndef RDF4CPP_XMLPARSERSTATECOLLECTOR_H
#define RDF4CPP_XMLPARSERSTATECOLLECTOR_H

#include <deque>
#include <iterator>
#include <memory>

#include <rdf4cpp/Expected.hpp>

#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>
#include <rdf4cpp/parser/ParsingError.hpp>
#include <rdf4cpp/parser/ParsingState.hpp>

#include <libxml/parser.h>

#include <dice/sparse-map/sparse_set.hpp>

#include <dice/template-library/inplace_polymorphic.hpp>

namespace rdf4cpp::parser {
    struct IStreamQuadIterator::ImplXMLStateCollector : Impl {
    protected:
        static std::string_view from_xml_char(xmlChar const *s) {
            if (s == nullptr) {
                return "";
            }
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return {reinterpret_cast<char const *>(s)};
        }

        static std::string_view from_xml_char(xmlChar const *s, xmlChar const *e) {
            if (s == nullptr) {
                return "";
            }
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return {reinterpret_cast<char const *>(s), reinterpret_cast<char const *>(e)};
        }

        static std::string_view from_xml_char(xmlChar const *s, int const n) {
            if (s == nullptr) {
                return "";
            }
            // ReSharper disable once CppDFALocalValueEscapesFunction
            return {reinterpret_cast<char const *>(s), static_cast<size_t>(n)};
        }

        struct Attribute {
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

        struct BaseState;

        struct InitialState;

        struct RDFState;

        struct DescriptionState;

        struct PredicateState;

        struct TypedLiteralPredicateState;

        struct XMLLiteralState;

        struct CollectionState;

        struct EmptyElement;
    };
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_XMLPARSERSTATECOLLECTOR_H
