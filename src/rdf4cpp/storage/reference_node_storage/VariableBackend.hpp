#ifndef RDF4CPP_VARIABLEBACKEND_HPP
#define RDF4CPP_VARIABLEBACKEND_HPP

#include <rdf4cpp/storage/view/VariableBackendView.hpp>

#include <dice/template-library/static_string.hpp>

namespace rdf4cpp::storage::reference_node_storage {

struct VariableBackend {
    using view_type = view::VariableBackendView;
    using id_type = identifier::NodeID;

    size_t hash;
    dice::template_library::static_string name;
    bool is_anonymous;

    explicit VariableBackend(view_type const &view) noexcept : hash{view.hash()},
                                                               name{view.name},
                                                               is_anonymous{view.is_anonymous} {
    }

    explicit operator view_type() const noexcept {
        return view_type{.name = name,
                         .is_anonymous = is_anonymous};
    }

    static identifier::NodeBackendID from_storage_id(id_type const id, [[maybe_unused]] view_type const view) noexcept {
        return identifier::NodeBackendID{id, identifier::RDFNodeType::Variable};
    }

    static id_type to_storage_id(identifier::NodeBackendID const id) noexcept {
        return id.node_id();
    }
};

}  // namespace rdf4cpp::storage::reference_node_storage

#endif  //RDF4CPP_VARIABLEBACKEND_HPP
