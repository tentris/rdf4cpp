#ifndef RDF4CPP_TRIPLEPATTERN_HPP
#define RDF4CPP_TRIPLEPATTERN_HPP

#include <rdf4cpp/Node.hpp>

#include <array>
#include <ostream>

namespace rdf4cpp::writer {
struct SerializationState;
} // namespace rdf4cpp::writer

namespace rdf4cpp::query {

/**
 * <div>TriplePattern</div> is modeled around SPARQL Triple patterns.
 *
 * One important difference is that pattern matching with <div>BlankNode</div>s is allowed for subject and object.
 * For the equivalent of a SPARQL anonymous variables, use Variable and set anonymous to true.
 *
 * @see <https://www.w3.org/TR/2013/REC-sparql11-update-20130321/#def_datasetQuadPattern>
 */
struct TriplePattern {
    using value_type = Node;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

protected:
    using storage_type = std::array<Node, 3>;
    storage_type entries_;

public:
    TriplePattern() noexcept = default;
    TriplePattern(Node subject, Node predicate, Node object) noexcept;

    [[nodiscard]] reference subject() noexcept { return entries_[0]; }
    [[nodiscard]] const_reference subject() const noexcept { return entries_[0]; }

    [[nodiscard]] reference predicate() noexcept { return entries_[1]; }
    [[nodiscard]] const_reference predicate() const noexcept { return entries_[1]; }

    [[nodiscard]] reference object() noexcept { return entries_[2]; }
    [[nodiscard]] const_reference object() const noexcept { return entries_[2]; }

    [[nodiscard]] reference operator[](size_type ix) noexcept { return entries_[ix]; }
    [[nodiscard]] const_reference operator[](size_type ix) const noexcept { return entries_[ix]; }

    [[nodiscard]] size_type size() const noexcept { return entries_.size(); }

    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;
    using reverse_iterator = typename storage_type::reverse_iterator;
    using const_reverse_iterator = typename storage_type::const_reverse_iterator;

    [[nodiscard]] iterator begin() noexcept { return entries_.begin(); }
    [[nodiscard]] const_iterator begin() const noexcept { return entries_.begin(); }
    [[nodiscard]] iterator end() noexcept { return entries_.end(); }
    [[nodiscard]] const_iterator end() const noexcept { return entries_.end(); }
    [[nodiscard]] reverse_iterator rbegin() noexcept { return entries_.rbegin(); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return entries_.rbegin(); }
    [[nodiscard]] reverse_iterator rend() noexcept { return entries_.rend(); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return entries_.rend(); }

    [[nodiscard]] bool valid() const noexcept;

    auto operator<=>(TriplePattern const &rhs) const = default;

    [[nodiscard]] explicit operator std::string() const;
    friend std::ostream &operator<<(std::ostream &os, TriplePattern const &pattern);

    [[nodiscard]] TriplePattern to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] TriplePattern try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const;
};
}  // namespace rdf4cpp::query


#endif  //RDF4CPP_TRIPLEPATTERN_HPP
