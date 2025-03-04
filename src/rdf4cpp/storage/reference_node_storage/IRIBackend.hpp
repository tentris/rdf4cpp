#ifndef RDF4CPP_IRIBACKEND_HPP
#define RDF4CPP_IRIBACKEND_HPP

#include <rdf4cpp/storage/view/IRIBackendView.hpp>

#include <dice/template-library/static_string.hpp>

namespace rdf4cpp::storage::reference_node_storage {

struct IRIBackend {
    using view_type = view::IRIBackendView;
    using id_type = identifier::NodeID;

    size_t hash;
    dice::template_library::static_string identifier;

    explicit IRIBackend(view_type const &view) noexcept : hash{view.hash()},
                                                          identifier{view.identifier} {
    }

    explicit operator view_type() const noexcept {
        return view_type{.identifier = identifier};
    }

    static identifier::NodeBackendID from_storage_id(id_type const id, [[maybe_unused]] view_type const view) noexcept {
        return identifier::NodeBackendID{id, identifier::RDFNodeType::IRI};
    }

    static id_type to_storage_id(identifier::NodeBackendID const id) noexcept {
        return id.node_id();
    }
};

}  // namespace rdf4cpp::storage::reference_node_storage

#endif  //RDF4CPP_IRIBACKEND_HPP
