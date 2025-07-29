#include "Solution.hpp"

namespace rdf4cpp::query {

template<typename Pat>
std::vector<Variable> Solution::extract_variables(Pat const &quad) {
    std::vector<Variable> variables;
    for (const auto &entry : quad) {
        if (entry.is_variable())
            variables.push_back(entry.as_variable());
    }
    return variables;
}

std::vector<std::pair<Variable, Node>> init(std::vector<Variable> const &variables) {
    std::vector<std::pair<Variable, Node>> partial_mapping;
    for (const auto &var_name : variables)
        partial_mapping.emplace_back(var_name, Node());
    return partial_mapping;
}

Solution::Solution(std::vector<Variable> const &variables) : partial_mapping{init(variables)} {}
Solution::Solution(QuadPattern const &qp) : Solution{extract_variables(qp)} {}
Solution::Solution(TriplePattern const &tp) : Solution{extract_variables(tp)} {}

Node Solution::operator[](Variable const &variable) const noexcept {
    size_t pos = std::distance(partial_mapping.begin(), std::find_if(partial_mapping.begin(), partial_mapping.end(),
                                                                     [=](const auto &item) { return item.first == variable; }));
    if (pos < partial_mapping.size()) {
        return partial_mapping[pos].second;
    } else {
        return {};
    }
}

Node const &Solution::operator[](size_t pos) const noexcept {
    RDF4CPP_ASSERT(pos < partial_mapping.size());
    return partial_mapping[pos].second;
}

Node &Solution::operator[](size_t pos) noexcept {
    RDF4CPP_ASSERT(pos < partial_mapping.size());
    return partial_mapping[pos].second;
}

Variable const &Solution::variable(size_t pos) const noexcept {
    RDF4CPP_ASSERT(pos < partial_mapping.size());
    return partial_mapping[pos].first;
}

size_t Solution::variable_count() const noexcept {
    return partial_mapping.size();
}

size_t Solution::bound_count() const noexcept {
    return std::count_if(partial_mapping.begin(), partial_mapping.end(),
                         [](const auto &item) { return not item.second.null(); });
}

Solution::iterator Solution::begin() noexcept {
    return partial_mapping.begin();
}

Solution::iterator Solution::end() noexcept {
    return partial_mapping.end();
}

Solution::const_iterator Solution::begin() const noexcept {
    return partial_mapping.begin();
}

Solution::const_iterator Solution::end() const noexcept {
    return partial_mapping.end();
}

}  // namespace rdf4cpp::query