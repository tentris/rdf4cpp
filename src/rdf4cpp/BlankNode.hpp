#ifndef RDF4CPP_BLANKNODE_HPP
#define RDF4CPP_BLANKNODE_HPP

#include <ostream>
#include <rdf4cpp/Node.hpp>
#include <rdf4cpp/CowString.hpp>

namespace rdf4cpp {

struct BlankNode : Node {
    /**
     * Constructs the null-bnode
     */
    BlankNode() noexcept;

    explicit BlankNode(storage::identifier::NodeBackendHandle handle) noexcept;

    /**
     * Constructs a bnode from an identifier
     */
    explicit BlankNode(std::string_view identifier, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * Constructs the null-bnode
     */
    [[nodiscard]] static BlankNode make_null() noexcept;

    /**
     * Constructs a bnode from an identifier
     */
    [[nodiscard]] static BlankNode make(std::string_view identifier, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * creates a new BlankNode from a random UUID (Universally Unique IDentifier)
     * @return UUID IRI
     */
    [[nodiscard]] static BlankNode make_uuid(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);


    /**
     * Constructs a bnode from an identifier
     */
    [[nodiscard]] static BlankNode make_unchecked(std::string_view identifier, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    BlankNode to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] BlankNode try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const;

    /**
     * searches for a bnode in the specified node storage and returns it.
     * returns a null bnode, if not found.
     * @param iri
     * @param node_storage
     * @return
     */
    [[nodiscard]] static BlankNode find(std::string_view identifier, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * Validates the given blank node identifier
     * @param identifier identifier to validate
     * @throws ParsingError if the blank node identifier is not valid
     */
    static void validate(std::string_view identifier);

    /**
     * Get the string identifier of this. For BlankNode `_:abc` the identifier is `abc`.
     * @return string identifier
     */
    [[nodiscard]] CowString identifier() const;

    /**
     * @see Literal::fetch_or_serialize_lexical_form
     */
    [[nodiscard]] FetchOrSerializeResult fetch_or_serialize_identifier(std::string_view &out, writer::BufWriterParts writer) const;

    /**
     * See Node::serialize
     */
    bool serialize(writer::BufWriterParts writer) const;

    [[nodiscard]] std::strong_ordering order(BlankNode const &other) const;

    [[nodiscard]] bool order_eq(BlankNode const &other) const;
    [[nodiscard]] bool order_ne(BlankNode const &other) const;

    [[nodiscard]] bool eq(BlankNode const &other) const;
    [[nodiscard]] bool ne(BlankNode const &other) const;

    [[nodiscard]] explicit operator std::string() const;

    friend std::ostream &operator<<(std::ostream &os, BlankNode const &node);

    bool is_literal() const noexcept = delete;
    bool is_variable() const noexcept = delete;
    bool is_blank_node() const noexcept = delete;
    bool is_iri() const noexcept = delete;

    friend struct Node;
};

inline namespace shorthands {

BlankNode operator""_bnode(char const *str, size_t len);

}  // namespace shorthands
}  // namespace rdf4cpp

template<>
struct std::hash<rdf4cpp::BlankNode> {
    inline size_t operator()(rdf4cpp::BlankNode const &v) const noexcept {
        return std::hash<rdf4cpp::Node>()(v);
    }
};

template<>
struct std::formatter<rdf4cpp::BlankNode> : std::formatter<rdf4cpp::Node> {
    auto format(rdf4cpp::BlankNode n, format_context &ctx) const -> decltype(ctx.out());
};

#endif  //RDF4CPP_BLANKNODE_HPP
