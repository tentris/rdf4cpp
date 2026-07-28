#ifndef RDF4CPP_QUADPATTERN_HPP
#define RDF4CPP_QUADPATTERN_HPP

#include <rdf4cpp/Node.hpp>
#include <rdf4cpp/query/TriplePattern.hpp>

#include <array>
#include <ostream>

namespace rdf4cpp::query {
/**
 * <div>QuadPattern</div> is modeled around SPARQL qual patterns.
 *
 * One important difference is that pattern matching with <div>BlankNode</div>s is allowed subject and object.
 * For the equivalent of SPARQL anonymous variables, use Variable and set anonymous to true.
 *
 * @see <https://www.w3.org/TR/sparql11-query/#defn_TriplePattern>
 */
struct QuadPattern {
    using value_type = Node;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

protected:
    using storage_type = std::array<Node, 4>;
    storage_type entries_{};

public:
    QuadPattern() noexcept = default;
    QuadPattern(Node graph, Node subject, Node predicate, Node object) noexcept;

    [[nodiscard]] reference graph() noexcept { return entries_[0]; }
    [[nodiscard]] const_reference graph() const noexcept { return entries_[0]; }

    [[nodiscard]] reference subject() noexcept { return entries_[1]; }
    [[nodiscard]] const_reference subject() const noexcept { return entries_[1]; }

    [[nodiscard]] reference predicate() noexcept { return entries_[2]; }
    [[nodiscard]] const_reference predicate() const noexcept { return entries_[2]; }

    [[nodiscard]] reference object() noexcept { return entries_[3]; }
    [[nodiscard]] const_reference object() const noexcept { return entries_[3]; }

    [[nodiscard]] reference operator[](size_type ix) noexcept { return entries_[ix]; }
    [[nodiscard]] const_reference operator[](size_type ix) const noexcept { return entries_[ix]; }

    [[nodiscard]] size_type size() const noexcept { return entries_.size(); }

    [[nodiscard]] bool valid() const noexcept;

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

    [[nodiscard]] TriplePattern const &without_graph() const noexcept;

    auto operator<=>(QuadPattern const &rhs) const = default;

    [[nodiscard]] explicit operator std::string() const;
    friend std::ostream &operator<<(std::ostream &os, QuadPattern const &pattern);

    [[nodiscard]] QuadPattern to_node_storage(storage::DynNodeStoragePtr node_storage) const;
    [[nodiscard]] QuadPattern try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const;
};
}  // namespace rdf4cpp::query

#endif  //RDF4CPP_QUADPATTERN_HPP
