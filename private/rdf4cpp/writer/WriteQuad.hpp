#ifndef RDF4CPP_PRIVATE_SERIALIZEQUAD_HPP
#define RDF4CPP_PRIVATE_SERIALIZEQUAD_HPP

#include <rdf4cpp/writer/OutputFormat.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/writer/SerializationState.hpp>

namespace rdf4cpp::writer {

template<writer::OutputFormat F>
bool write_node(Node const node, writer::BufWriterParts const writer) noexcept {
    if constexpr (writer::format_has_prefix<F>) {
        return node.serialize(writer, NodeSerializationOpts::prefixed_and_short_form());
    } else {
        return node.serialize(writer, NodeSerializationOpts::long_form());
    }
}

template<writer::OutputFormat F>
bool write_pred(Node const pred, writer::BufWriterParts const writer) {
    if constexpr (writer::format_has_prefix<F>) {
        if (pred.as_iri().is_rdf_type()) {
            return writer::write_str("a", writer);
        }
    }

    return write_node<F>(pred, writer);
}

#define RDF4CPP_DETAIL_TRY_WRITE_NODE(pred) \
    if (!write_node<F>((pred), writer)) {   \
        return false;                       \
    }

#define RDF4CPP_DETAIL_TRY_WRITE_PRED(pred) \
    if (!write_pred<F>((pred), writer)) {   \
        return false;                       \
    }

[[nodiscard]] inline bool print_graph(Node const &graph) noexcept {
    return !graph.null() && graph.as_iri().is_default_graph() != TriBool::True;
}

template<writer::OutputFormat F, typename Q>
bool write_quad(Q const &s, writer::BufWriterParts const writer, writer::SerializationState *const state) noexcept {
    if constexpr (writer::format_has_prefix<F>) {
        if constexpr (writer::format_has_graph<F>) {
            if (s.graph() != state->active_graph) {
                if (!state->flush(writer)) {
                    return false;
                }

                if (print_graph(s.graph())) {
                    RDF4CPP_DETAIL_TRY_WRITE_NODE(s.graph());
                    RDF4CPP_DETAIL_TRY_WRITE_STR(" {\n");

                    state->active_graph = s.graph();
                }
            }
        }

        if (!state->active_subject.null() && state->active_subject == s.subject()) {
            if (!state->active_predicate.null() && state->active_predicate == s.predicate()) {
                RDF4CPP_DETAIL_TRY_WRITE_STR(" ,\n");
                RDF4CPP_DETAIL_TRY_WRITE_NODE(s.object());
                return true;
            }

            RDF4CPP_DETAIL_TRY_WRITE_STR(" ;\n");
            RDF4CPP_DETAIL_TRY_WRITE_PRED(s.predicate());

            state->active_predicate = s.predicate();
            RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
            RDF4CPP_DETAIL_TRY_WRITE_NODE(s.object());
            return true;
        }

        if (!state->active_subject.null()) {
            RDF4CPP_DETAIL_TRY_WRITE_STR(" .\n");
        }

        state->active_subject = s.subject();
        state->active_predicate = s.predicate();
    }

    RDF4CPP_DETAIL_TRY_WRITE_NODE(s.subject());
    RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
    RDF4CPP_DETAIL_TRY_WRITE_PRED(s.predicate());
    RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
    RDF4CPP_DETAIL_TRY_WRITE_NODE(s.object());

    if constexpr (!writer::format_has_prefix<F>) {
        if constexpr (writer::format_has_graph<F>) {
            if (print_graph(s.graph())) {
                RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
                RDF4CPP_DETAIL_TRY_WRITE_NODE(s.graph());
            }
        }

        RDF4CPP_DETAIL_TRY_WRITE_STR(" .\n");
    }

    return true;
}

#undef RDF4CPP_DETAIL_TRY_WRITE_NODE
#undef RDF4CPP_DETAIL_TRY_WRITE_PRED

} // namespace rdf4cpp::writer

#endif // RDF4CPP_PRIVATE_SERIALIZEQUAD_HPP
