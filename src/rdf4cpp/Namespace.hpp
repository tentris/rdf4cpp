#ifndef RDF4CPP_NAMESPACE_HPP
#define RDF4CPP_NAMESPACE_HPP

#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/util/TransparentStringHash.hpp>

#include <dice/sparse-map/sparse_map.hpp>

#include <string>
#include <string_view>

namespace rdf4cpp {

/**
 * Namespace provides a simple tool to create IRIs from a certain namespace and vocabulary. Each Namespace instance maintains an internal cache to save roundtrip to storage::NodeStorage.<br/>
 * Example:
 *  @code
 *  Namespace ex("http://example.com/);
 *  IRI me = ex + "me";
 *  @endcode
 */
struct Namespace {
protected:
    /**
     * The IRI string of the namespace.
     */
    std::string namespace_iri_;

    /**
     * NodeStorage from which IRI objects are created.
     */
    storage::DynNodeStoragePtr node_storage_;

    // TODO: a faster, less memory efficient map would be better.
    /**
     * Cache storing the <div>NodeBackendHandle</div> for prefixes. This saves roundtrips to NodeStorage.
     */
    mutable dice::sparse_map::sparse_map<std::string, storage::identifier::NodeBackendID,
                                         util::TransparentStringHash, std::equal_to<>> cache_;

public:
    /**
     * Namespace Constructor
     * @param namespace_iri namespace IRI string. This will be used as prefix. IRI must not be encapsulated in <..>.
     * @param node_storage where the IRIs will live
     */
    explicit Namespace(std::string_view namespace_iri,
                       storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * @return IRI string of the Namespace
     */
    std::string_view name_space() const noexcept;

    /**
     * @return NodeStorage used to create <div>IRI</div>s from this Namespace.
     */
    storage::DynNodeStoragePtr node_storage() const;

    /**
     * Create an IRI with the suffix added to the Namespace.
     * @param suffix suffix that is appended
     * @return the constructed IRI
     */
    virtual IRI operator+(std::string_view suffix) const;

    /**
     * Clears the cache.
     */
    virtual void clear() const;
};
}  // namespace rdf4cpp

#endif  //RDF4CPP_NAMESPACE_HPP
