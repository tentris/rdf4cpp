#ifndef RDF4CPP_FALLBACKLITERALBACKEND_HPP
#define RDF4CPP_FALLBACKLITERALBACKEND_HPP

#include <rdf4cpp/storage/identifier/NodeID.hpp>
#include <rdf4cpp/storage/view/LiteralBackendView.hpp>

#include <dice/template-library/static_string.hpp>

namespace rdf4cpp::storage::reference_node_storage {

struct FallbackLiteralBackend {
    using view_type = view::LexicalFormLiteralBackendView;
    using id_type = identifier::LiteralID;

    size_t hash;
    identifier::NodeBackendID datatype_id;
    dice::template_library::static_string lexical_form;
    dice::template_library::static_string language_tag;
    bool needs_escape;

    explicit FallbackLiteralBackend(view_type const &view) noexcept : hash{view.hash()},
                                                                      datatype_id{view.datatype_id},
                                                                      lexical_form{view.lexical_form},
                                                                      language_tag{view.language_tag},
                                                                      needs_escape{view.needs_escape} {
    }

    explicit operator view_type() const noexcept {
        return view_type{.datatype_id = datatype_id,
                         .lexical_form = lexical_form,
                         .language_tag = language_tag,
                         .needs_escape = needs_escape};
    }

    static identifier::NodeBackendID from_storage_id(id_type const id, view_type const view) noexcept {
        return identifier::NodeBackendID{identifier::NodeID{id, iri_node_id_to_literal_type(view.datatype_id)}, identifier::RDFNodeType::Literal};
    }

    static id_type to_storage_id(identifier::NodeBackendID const id) noexcept {
        return id.node_id().literal_id();
    }

    static view_type get_default_view() noexcept {
        auto const default_iri_fixed_id = datatypes::registry::reserved_datatype_ids[datatypes::registry::default_graph_iri];
        auto const default_iri_node_id = identifier::literal_type_to_iri_node_id(default_iri_fixed_id);

        return view_type{.datatype_id = default_iri_node_id, .lexical_form = "", .language_tag = "", .needs_escape = false};
    }
};

}  // namespace rdf4cpp::storage::view

#endif  //RDF4CPP_FALLBACKLITERALBACKEND_HPP
