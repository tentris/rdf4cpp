#include <rdf4cpp/parser/XMLParser.hpp>

namespace rdf4cpp::parser {
    void IStreamQuadIterator::ImplXML::XMLLiteralState::on_characters(ImplXML &i, [[maybe_unused]] std::string_view chars) {
        source_input(i);
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::on_start_element(ImplXML &i, [[maybe_unused]] std::string_view local_name, [[maybe_unused]] std::string_view uri, [[maybe_unused]] std::span<Attribute> attributes) {
        ++depth;
        source_input(i);
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::on_end_element(ImplXML &i) {
        if (depth > 0) {
            --depth;
            source_input(i);
            return;
        }
        IRI datatype = i.make_hardcoded_iri("http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral");
        std::string_view l = literal;
        l = l.substr(0, last_offset);
        l.remove_prefix(data_start);
        if (!l.empty() && l[0] == '/') {
            l.remove_prefix(1);
        }
        if (!l.empty() && l[0] == '>') {
            l.remove_prefix(1);
        }
        Literal const lit = i.make_literal(l, datatype, std::nullopt);
        i.add_statement(subject, predicate, lit, reify);
        i.pop_state();
    }
    void IStreamQuadIterator::ImplXML::XMLLiteralState::move_to(BaseState *b) noexcept {
        new (b) XMLLiteralState(std::move(*this));
    }

    void IStreamQuadIterator::ImplXML::XMLLiteralState::source_input(ImplXML &i) {
        xmlChar const *data;
        int size = 1024;
        int off = 0;
        xmlCtxtGetInputWindow(i.context_.get(), 0, &data, &size, &off);
        std::string_view const sv{reinterpret_cast<char const *>(data), static_cast<size_t>(size)};
        if (literal.empty()) {
            data_start = off;
        }
        if (!static_cast<std::string_view>(literal).ends_with(sv)) {
            last_size = literal.size();
            literal += sv;
        }
        last_offset = static_cast<size_t>(off) + last_size;
    }
}