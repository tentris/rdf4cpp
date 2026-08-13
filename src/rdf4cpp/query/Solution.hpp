#ifndef RDF4CPP_SOLUTION_HPP
#define RDF4CPP_SOLUTION_HPP

#include <rdf4cpp/query/QuadPattern.hpp>
#include <rdf4cpp/query/TriplePattern.hpp>

namespace rdf4cpp::query {

struct Solution {
private:
    using storage_type = std::vector<std::pair<Variable, Node>>;

public:
    using value_type = std::pair<Variable, Node>;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using iterator = typename storage_type::iterator;
    using const_iterator = typename storage_type::const_iterator;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

private:
    storage_type partial_mapping;

    template<typename Pat>
    static std::vector<Variable> extract_variables(Pat const &pat);

public:
    Solution() noexcept = default;

    explicit Solution(std::vector<Variable> const &variables);
    explicit Solution(QuadPattern const &qp);
    explicit Solution(TriplePattern const &tp);

    Node operator[](Variable const &variable) const;

    Node const &operator[](size_t pos) const noexcept;
    Node &operator[](size_t pos) noexcept;

    [[nodiscard]] Variable const &variable(size_t pos) const noexcept;

    [[nodiscard]] size_t variable_count() const noexcept;
    [[nodiscard]] size_t bound_count() const noexcept;

    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] iterator end() noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
};
}  // namespace rdf4cpp::query

#endif  //RDF4CPP_SOLUTION_HPP
