#include "Namespace.hpp"

namespace rdf4cpp {

Namespace::Namespace(std::string_view namespace_iri, storage::DynNodeStoragePtr node_storage)
    : namespace_iri_{namespace_iri}, node_storage_{node_storage} {
}

std::string_view Namespace::name_space() const noexcept {
    return namespace_iri_;
}

storage::DynNodeStoragePtr Namespace::node_storage() const {
    return node_storage_;
}

IRI Namespace::operator+(std::string_view suffix) const {
    if (auto found = cache_.find(suffix); found != cache_.end()) {
        return IRI{storage::identifier::NodeBackendHandle{found->second, node_storage_}};
    } else {
        return [&]() {
            std::string namespace_iri{namespace_iri_};
            namespace_iri.append(suffix);

            IRI iri{namespace_iri, node_storage_};
            cache_.emplace(suffix, iri.backend_handle().id());
            return iri;
        }();
    }
}

void Namespace::clear() const {
    cache_.clear();
    cache_.rehash(0);
}

}  // namespace rdf4cpp
