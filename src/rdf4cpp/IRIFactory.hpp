#ifndef RDF4CPP_IRIFACTORY_HPP
#define RDF4CPP_IRIFACTORY_HPP

#include <string>
#include <string_view>
#include <boost/container/flat_map.hpp>

#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/IRIView.hpp>

namespace rdf4cpp {
/**
 * Stores a base IRI and a prefix map and allows to create IRIs by possibly applying both.
 */
struct IRIFactory {
    using prefix_map_type = boost::container::flat_map<std::string, std::string, std::less<>>;

private:
    prefix_map_type prefixes;

    std::string base;
    IRIView::AllParts base_parts_cache;

public:
    constexpr static std::string_view default_base = "http://example.org/";
    /**
     * Creates a IRIFactory with empty prefix map and a given base IRI. Throws if base is invalid.
     * @param base
     */
    explicit IRIFactory(std::string_view base = default_base);
    /**
     * Creates a IRIFactory with a given prefix map and a given base IRI. Throws if base is invalid.
     * @param prefixes
     * @param base
     */
    explicit IRIFactory(prefix_map_type &&prefixes, std::string_view base = default_base);

    IRIFactory(IRIFactory &&) noexcept = default;
    IRIFactory &operator=(IRIFactory &&) noexcept = default;

    // provide only const iterators to ensure that no key/values will be changed
    using const_iterator = prefix_map_type::const_iterator;
    using const_reverse_iterator = prefix_map_type::const_reverse_iterator;

    [[nodiscard]] const_iterator begin() const noexcept { return prefixes.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return prefixes.end(); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return prefixes.rbegin(); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return prefixes.rend(); }

    /**
     * Creates a IRI from a relative IRI.
     * Implements https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.
     * @param rel
     * @param storage
     * @return
     */
    [[nodiscard]] nonstd::expected<IRI, IRIFactoryError> from_relative(std::string_view rel, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) const noexcept;
    /**
     * Creates a IRI from a possibly relative IRI.
     * if rel is relative, returns the same as from_relative, otherwise returns rel unchanged.
     * @param rel
     * @param storage
     * @return
     */
    [[nodiscard]] nonstd::expected<IRI, IRIFactoryError> from_maybe_relative(std::string_view rel, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) const noexcept;
    /**
     * Creates a IRI by looking up a prefix in the prefix map and possibly resolving a relative IRI.
     * @param prefix
     * @param local
     * @param storage
     * @return
     */
    [[nodiscard]] nonstd::expected<IRI, IRIFactoryError> from_prefix(std::string_view prefix, std::string_view local, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) const;

    /**
     * Creates or changes a prefix.
     * @param prefix
     * @param expanded
     */
    [[nodiscard]] IRIFactoryError assign_prefix(std::string_view prefix, std::string_view expanded);

    /**
     * Creates or changes a prefix.
     * expects prefix and expanded to be valid (i.e. not contain spaces or other chars that are invalid in an IRI).
     * this is ensured by assign_prefix
     * @param prefix
     * @param expanded
     */
    void assign_prefix_unchecked(std::string_view prefix, std::string_view expanded);
    /**
     * Removes a prefix.
     * @param prefix
     */
    void clear_prefix(std::string_view prefix);

    /**
     * The base IRI.
     * @return
     */
    [[nodiscard]] std::string_view get_base() const noexcept;
    /**
     * Changes the base IRI. Validates the new base IRI before setting.
     * @param b
     * @return
     */
    [[nodiscard]] IRIFactoryError set_base(std::string_view b) noexcept;
    /**
     * Changes the base IRI. Skips validating the new base IRI.
     * @param b
     * @return
     */
    void set_base_unchecked(std::string_view b) noexcept;

    /**
     * validates the given IRI and creates it in the given node storage, if valid.
     * @param iri
     * @param storage
     * @return
     */
    [[nodiscard]] static nonstd::expected<IRI, IRIFactoryError> create_and_validate(std::string_view iri, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) noexcept;
};

}  // namespace rdf4cpp
#endif  //RDF4CPP_IRIFACTORY_HPP
