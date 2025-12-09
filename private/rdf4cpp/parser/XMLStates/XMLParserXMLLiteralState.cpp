#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::XMLLiteralState::on_characters([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] std::string_view chars, Info const &info) {
        source_input(info);
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::XMLLiteralState::on_start_element([[maybe_unused]] XMLOutputQueue &out, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes, Info const &info) {
        ++depth;
        source_input(info);
        return {};
    }

    IStreamQuadIterator::ImplXMLStateCollector::StateTransition IStreamQuadIterator::ImplXMLStateCollector::XMLLiteralState::on_end_element(XMLOutputQueue &out, Info const &info) {
        if (depth > 0) {
            --depth;
            source_input(info);
            return {};
        }
        IRI datatype = out.make_hardcoded_iri("http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral");
        std::string_view l = literal;
        l = l.substr(0, last_offset);
        l.remove_prefix(data_start);
        if (!l.empty() && l[0] == '/') {
            l.remove_prefix(1);
        }
        if (!l.empty() && l[0] == '>') {
            l.remove_prefix(1);
        }
        Literal const lit = out.make_literal(l, datatype, std::nullopt, info);
        out.add_statement(subject, predicate, lit, reify);
        return StateTransition{std::in_place_type_t<PopState>{}};
    }
    void IStreamQuadIterator::ImplXMLStateCollector::XMLLiteralState::move_to(BaseState *b) noexcept {
        new (b) XMLLiteralState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXMLStateCollector::XMLLiteralState::source_input(Info const &info) {
        int const off = info.source_offset;
        std::string_view const sv = info.source;
        if (literal.empty()) {
            data_start = off;
        }
        if (!static_cast<std::string_view>(literal).ends_with(sv)) {
            last_size = literal.size();
            literal += sv;
        }
        last_offset = static_cast<size_t>(off) + last_size;
    }
}  // namespace rdf4cpp::parser