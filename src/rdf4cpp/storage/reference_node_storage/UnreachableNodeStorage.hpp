#ifndef RDF4CPP_STORAGE_UNREACHABLENODESTORAGE_HPP
#define RDF4CPP_STORAGE_UNREACHABLENODESTORAGE_HPP

#include <rdf4cpp/storage/reference_node_storage/BNodeBackend.hpp>
#include <rdf4cpp/storage/reference_node_storage/FallbackLiteralBackend.hpp>
#include <rdf4cpp/storage/reference_node_storage/IRIBackend.hpp>
#include <rdf4cpp/storage/reference_node_storage/VariableBackend.hpp>
#include <stdexcept>

namespace rdf4cpp::storage::reference_node_storage {

struct UnreachableNodeStorage {
    [[nodiscard]] static bool has_specialized_storage_for([[maybe_unused]] identifier::LiteralType datatype) {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id([[maybe_unused]] view::BNodeBackendView const &view) {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id([[maybe_unused]] view::IRIBackendView const &view) {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id([[maybe_unused]] view::LiteralBackendView const &view) {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_or_make_id([[maybe_unused]] view::VariableBackendView const &view) {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_id([[maybe_unused]] view::BNodeBackendView const &view) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }
    [[nodiscard]] identifier::NodeBackendID find_id([[maybe_unused]] view::IRIBackendView const &view) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_id([[maybe_unused]] view::LiteralBackendView const &view) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] identifier::NodeBackendID find_id([[maybe_unused]] view::VariableBackendView const &view) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] view::IRIBackendView find_iri_backend([[maybe_unused]] identifier::NodeBackendID id) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] view::LiteralBackendView find_literal_backend([[maybe_unused]] identifier::NodeBackendID id) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] view::BNodeBackendView find_bnode_backend([[maybe_unused]] identifier::NodeBackendID id) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }

    [[nodiscard]] view::VariableBackendView find_variable_backend([[maybe_unused]] identifier::NodeBackendID id) const {
        throw std::logic_error("UnreachableNodeStorage cannot be used");
    }
};

}  // namespace rdf4cpp::storage::reference_node_storage

#endif  //RDF4CPP_UNREACHABLENODESTORAGE_H