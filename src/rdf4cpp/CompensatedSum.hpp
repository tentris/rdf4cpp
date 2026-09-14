#ifndef RDF4CPP_COMPENSATEDSUM_HPP
#define RDF4CPP_COMPENSATEDSUM_HPP

#include <rdf4cpp/DeferredLiteral.hpp>
#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/Literal.hpp>

namespace rdf4cpp {

/**
 * @brief Sums numeric Literals incrementally, compensating for the rounding error that the
 * inexact datatypes (xsd:float, xsd:double, owl:real) accumulate over a long sequence.
 * See https://en.wikipedia.org/wiki/Kahan_summation_algorithm#Further_enhancements
 *
 * @example
 * @code
 * CompensatedSum sum;
 * for (auto const &[lit, multiplicity] : rows) {
 *     sum.add(lit, multiplicity);
 * }
 * Literal const result = sum.value();
 * @endcode
 */
struct CompensatedSum {
private:
    storage::DynNodeStoragePtr node_storage_;

    std::optional<DeferredLiteral> sum_;  // running total (initially zero)
    DeferredLiteral comp_;                // low-order bits lost by the additions so far, the null-value until the first of them

    // latched as soon as an inexact datatype is involved, and never cleared because the numeric
    // hierarchy only ever widens: once the total is a double it stays one
    bool compensating_ = false;

    // exactness of the datatype added last, so that a stream of a single datatype (the common case)
    // costs one IRI comparison per element instead of a registry lookup
    IRI cached_datatype_;
    bool cached_exact_ = false;

    [[nodiscard]] bool is_exact(IRI const &datatype);

    void add_once(DeferredLiteral const &value);

public:
    /**
     * @param node_storage the node storage the datatype IRIs live in, and that value() places its
     *      result into. Intermediate values are never placed there.
     */
    explicit CompensatedSum(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    /**
     * Adds value multiplicity times
     */
    void add(Literal const &lit, uint64_t multiplicity = 1);
    void add(DeferredLiteral const &value, uint64_t multiplicity = 1);

    /**
     * @return the sum, or the null-literal if a non-numeric or null value was added. The sum of
     *      nothing is "0"^^xsd:integer.
     */
    [[nodiscard]] Literal value() const;

    /**
     * @return true if the sum will never return a non-null value again
     */
    [[nodiscard]] bool poisoned() const noexcept;

    /**
     * @return The result of a sum with no operands
     */
    [[nodiscard]] static Literal nullary_sum(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);
};

}  // namespace rdf4cpp

#endif  //RDF4CPP_COMPENSATEDSUM_HPP
