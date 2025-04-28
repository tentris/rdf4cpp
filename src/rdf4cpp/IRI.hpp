#ifndef RDF4CPP_IRI_HPP
#define RDF4CPP_IRI_HPP

#include <ostream>
#include <rdf4cpp/Node.hpp>
#include <rdf4cpp/datatypes/registry/DatatypeID.hpp>

namespace rdf4cpp {

/**
 * IRI Resource node.
 */
struct IRI : Node {
private:
    /**
     * Constructs the corresponding IRI from a given datatype id and places it into node_storage if
     * it does not exist already.
     */
    IRI(datatypes::registry::DatatypeIDView id, storage::DynNodeStoragePtr node_storage) noexcept;
    
    /**
     * Constructs the corresponding datatype id for this iri. Return value can be safely used to
     * index the registry and yields the correct result.
     */
    explicit operator datatypes::registry::DatatypeIDView() const noexcept;

public:
    explicit IRI(storage::identifier::NodeBackendHandle handle) noexcept;

    /**
     * Constructs the null-iri
     */
    IRI() noexcept;

    /**
     * Constructs an IRI object from a IRI string
     * @param iri IRI string
     * @param node_storage optional custom node_storage used to store the IRI
     * @throw rdf4cpp::ParsingError if iri is invalid
     */
    explicit IRI(std::string_view iri, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * Constructs the null-iri
     */
    [[nodiscard]] static IRI make_null() noexcept;

    /**
     * Constructs an IRI object from a IRI string.
     * @param iri IRI string
     * @param node_storage optional custom node_storage used to store the IRI
     * @throw rdf4cpp::ParsingError if iri is invalid
     */
    [[nodiscard]] static IRI make(std::string_view iri, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * Constructs an IRI object from a IRI string without checking if the IRI is valid.
     * @param iri IRI string
     * @param node_storage optional custom node_storage used to store the IRI
     */
    [[nodiscard]] static IRI make_unchecked(std::string_view iri, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * creates a new IRI containing a random UUID (Universally Unique IDentifier)
     * @return UUID IRI
     */
    [[nodiscard]] static IRI make_uuid(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    IRI to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] IRI try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept;

    /**
     * searches for a IRI in the specified node storage and returns it.
     * returns a null IRI, if not found.
     * @param iri
     * @param node_storage
     * @return
     */
    [[nodiscard]] static IRI find(std::string_view iri, storage::DynNodeStoragePtr node_storage = storage::default_node_storage) noexcept;

    /**
     * Validates that the given input string is a valid IRI
     * @param iri iri string to validate
     * @throws ParsingError if the given IRI is invalid
     */
    static void validate(std::string_view iri);

private:
    /**
     * searches for a IRI in the specified node storage and returns it.
     * returns a null IRI, if not found.
     * @param iri
     * @param node_storage
     * @return
     */
    [[nodiscard]] static IRI find(datatypes::registry::DatatypeIDView id, storage::DynNodeStoragePtr node_storage) noexcept;

public:
    /**
     * Get the IRI string of this.
     * @return IRI string
     */
    [[nodiscard]] std::string_view identifier() const noexcept;

    /**
     * @see Literal::fetch_or_serialize_lexical_form
     */
    [[nodiscard]] FetchOrSerializeResult fetch_or_serialize_identifier(std::string_view &out, writer::BufWriterParts writer) const noexcept;

    /**
     * See Node::serialize
     */
    bool serialize(writer::BufWriterParts writer) const noexcept;

    [[nodiscard]] explicit operator std::string() const noexcept;
    friend std::ostream &operator<<(std::ostream &os, const IRI &iri);

    bool is_literal() const noexcept = delete;
    bool is_variable() const noexcept = delete;
    bool is_blank_node() const noexcept = delete;
    bool is_iri() const noexcept = delete;

    friend struct Node;
    friend struct Literal;

    /**
     * Get the default graph IRI.
     * @param node_storage optional custom node_storage where the returned IRI lives
     * @return default graph IRI
     */
    [[nodiscard]] static IRI default_graph(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * @return err if this is null, otherwise true iff this IRI is the default graph IRI
     */
    [[nodiscard]] TriBool is_default_graph() const noexcept;

    /**
     * Get the IRI for rdf:type
     * @param node_storage optional custom node_storage where the returned IRI lives
     * @return rdf:type IRI
     */
    [[nodiscard]] static IRI rdf_type(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * @return err if this is null, otherwise true iff this IRI is rdf:type
     */
    [[nodiscard]] TriBool is_rdf_type() const noexcept;
};

inline namespace shorthands {

IRI operator""_iri(char const *str, size_t len);

}  // namespace shorthands
}  // namespace rdf4cpp

template<>
struct std::hash<rdf4cpp::IRI> {
    inline size_t operator()(rdf4cpp::IRI const &v) const noexcept {
        return std::hash<rdf4cpp::Node>()(v);
    }
};

template<>
struct std::formatter<rdf4cpp::IRI> : std::formatter<rdf4cpp::Node> {
    auto format(rdf4cpp::IRI n, format_context &ctx) const -> decltype(ctx.out());
};

#endif  //RDF4CPP_IRI_HPP
