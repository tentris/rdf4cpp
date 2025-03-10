#ifndef RDF4CPP_RDFNODETYPE_HPP
#define RDF4CPP_RDFNODETYPE_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace rdf4cpp::storage::identifier {
/**
 * Specifies whether an RDF node is a Variable, BlankNode, IRI or Literal.
 * The ordering BNode, IRI, Literal is the same as in SPARQL operator<.
 */
enum struct RDFNodeType : uint8_t {
    BNode = 0b00,
    IRI = 0b01,
    Literal = 0b10,
    Variable = 0b11,
};

constexpr std::string_view to_string_view(RDFNodeType const node_type) noexcept {
    switch (node_type) {
        case RDFNodeType::Variable: return "Variable";
        case RDFNodeType::BNode: return "BNode";
        case RDFNodeType::IRI: return "IRI";
        case RDFNodeType::Literal: return "Literal";
        default: return "Undefined";
    }
}

inline std::string to_string(RDFNodeType const node_type) noexcept {
    return std::string{to_string_view(node_type)};
}

inline std::ostream &operator<<(std::ostream &os, RDFNodeType type) {
    os << to_string_view(type);
    return os;
}

}  // namespace rdf4cpp::storage::identifier


#endif  //RDF4CPP_RDFNODETYPE_HPP
