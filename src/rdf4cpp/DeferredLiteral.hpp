#ifndef RDF4CPP_DEFERRED_LITERAL_HPP
#define RDF4CPP_DEFERRED_LITERAL_HPP

#include <rdf4cpp/Literal.hpp>
#include <rdf4cpp/IRI.hpp>

#include <any>

namespace rdf4cpp {

/**
 * @brief The value of a Literal that is not (yet) placed into a node storage, together with the IRI of its datatype.
 * A DeferredLiteral with a null datatype IRI is the null-value; it is what the numeric_*_deferred functions
 * return on error and it propagates through them, just like the null-Literal does for Literal::add and friends.
 * @warning the dynamic type of the value must be the cpp_type of the datatype, otherwise it is undefined behaviour
 * @note Use make_deferred_from_value or make_deferred_from_literal to construct one to avoid UB.
 */
struct DeferredLiteral {
    std::any value;
    IRI datatype;

    [[nodiscard]] bool null() const noexcept {
        return datatype.null();
    }
};

namespace deferred_detail {
[[nodiscard]] DeferredLiteral make_deferred_from_value(std::any value,
                                                       datatypes::registry::DatatypeIDView datatype,
                                                       storage::DynNodeStoragePtr node_storage);

template<typename OpSelect>
requires std::is_nothrow_invocable_r_v<datatypes::registry::DatatypeRegistry::binop_fptr_t,
                                       OpSelect,
                                       datatypes::registry::DatatypeRegistry::NumericOpsImpl const &>
[[nodiscard]] DeferredLiteral numeric_binop_deferred_impl(OpSelect op_select,
                                                          DeferredLiteral const &lhs,
                                                          DeferredLiteral const &rhs,
                                                          storage::DynNodeStoragePtr node_storage) {
    return numeric_equalized_deferred_impl(
        [&](datatypes::registry::DatatypeRegistry::NumericOpsImpl const &num_ops, std::any const &l, std::any const &r) -> DeferredLiteral {
            auto op_res = op_select(num_ops)(l, r);
            if (!op_res.result_value.has_value()) {
                return DeferredLiteral{};
            }

            return DeferredLiteral{std::move(*op_res.result_value), IRI{op_res.result_type_id, node_storage}};
        },
        DeferredLiteral{},
        lhs,
        rhs);
}

/**
 * Converts lhs and rhs to their common numeric datatype and applies apply_op to the numeric ops of
 * that datatype and the two converted values.
 * @return the result of apply_op, or fallback if either side is not numeric or the two have no common numeric type
 */
template<typename Result, typename ApplyOp>
requires std::is_invocable_r_v<Result, ApplyOp, datatypes::registry::DatatypeRegistry::NumericOpsImpl const &, std::any const &, std::any const &>
[[nodiscard]] Result numeric_equalized_deferred_impl(ApplyOp apply_op,
                                                     Result fallback,
                                                     DeferredLiteral const &lhs,
                                                     DeferredLiteral const &rhs) {
    using namespace datatypes::registry;

    if (lhs.null() || rhs.null()) {
        return fallback;
    }

    DatatypeIDView const lhs_datatype{lhs.datatype};
    auto const *lhs_entry = DatatypeRegistry::get_entry(lhs_datatype);
    if (lhs_entry == nullptr || !lhs_entry->numeric_ops.has_value()) {
        return fallback;  // not registered or not numeric
    }

    DatatypeIDView const rhs_datatype{rhs.datatype};

    if (lhs_datatype == rhs_datatype && lhs_entry->numeric_ops->is_impl()) {
        return apply_op(lhs_entry->numeric_ops->get_impl(), lhs.value, rhs.value);
    }

    auto const *rhs_entry = DatatypeRegistry::get_entry(rhs_datatype);
    if (rhs_entry == nullptr || !rhs_entry->numeric_ops.has_value()) {
        return fallback;  // not registered, or not numeric
    }

    auto const equalizer = DatatypeRegistry::get_common_numeric_op_type_conversion(*lhs_entry, *rhs_entry);
    if (!equalizer.has_value()) {
        return fallback;  // not convertible
    }

    auto const *equalized_entry = [&]() {
        if (equalizer->target_type_id == lhs_datatype) {
            return lhs_entry;
        }
        if (equalizer->target_type_id == rhs_datatype) {
            return rhs_entry;
        }
        return DatatypeRegistry::get_entry(equalizer->target_type_id);
    }();

    RDF4CPP_ASSERT(equalized_entry != nullptr);
    RDF4CPP_ASSERT(equalized_entry->numeric_ops.has_value());
    RDF4CPP_ASSERT(equalized_entry->numeric_ops->is_impl());

    return apply_op(equalized_entry->numeric_ops->get_impl(), equalizer->convert_lhs(lhs.value), equalizer->convert_rhs(rhs.value));
}
}  // namespace deferred_detail

/**
 * @brief Constructs a DeferredLiteral from a compatible type.
 * The datatype is specified at compile time, which guarantees that the value and the datatype match.
 * @tparam T the datatype
 * @param value instance for which the DeferredLiteral is created
 * @param node_storage node storage the datatype IRI is placed in
 */
template<datatypes::LiteralDatatype T>
[[nodiscard]] DeferredLiteral make_deferred_from_value(typename T::cpp_type const &value,
                                                       storage::DynNodeStoragePtr node_storage = storage::default_node_storage) {
    return deferred_detail::make_deferred_from_value(std::any{value}, T::datatype_id, node_storage);
}

/**
 * @return the value and the datatype of lit, or the null-value if lit is the null-literal
 */
[[nodiscard]] DeferredLiteral make_deferred_from_literal(Literal const &lit);

/**
 * Make a deferred literal by converting the given multiplicity to the given datatype
 *
 * @param multiplicity multiplicity
 * @param datatype datatype iri
 * @return deferred literal corresponding to the multiplicity (or null-literal if the multiplicity was not representable)
 */
[[nodiscard]] DeferredLiteral make_deferred_from_multiplicity(uint64_t multiplicity, IRI const &datatype);

/**
 * @brief Places a DeferredLiteral into node_storage
 * @return the resulting literal, or the null-literal if value is the null-value
 */
[[nodiscard]] Literal materialize_deferred(DeferredLiteral value, storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

/**
 * Numeric operations that do not place their result into a node storage.
 * They behave like the corresponding Literal member functions, except that
 *  - they do not store their (intermediate) results, and
 *  - only numeric datatypes are supported, timepoints and durations yield the null-value.
 *
 * Results are turned into Literals via materialize_deferred.
 *
 * @param node_storage the node storage the datatype IRI of the result lives in. Only the IRI is
 *      placed there, never the result value itself, and only if it is not already available there.
 *
 * @example folding without storing the intermediate results
 * @code
 * auto acc = make_deferred_from_value<datatypes::xsd::Integer>(0);
 * for (Literal const &lit : literals) {
 *     acc = numeric_add_deferred(acc, make_deferred_from_literal(lit));
 * }
 * Literal const sum = materialize_deferred(std::move(acc));
 * @endcode
 */
[[nodiscard]] DeferredLiteral numeric_add_deferred(DeferredLiteral const &lhs,
                                                   DeferredLiteral const &rhs,
                                                   storage::DynNodeStoragePtr node_storage = storage::default_node_storage);
[[nodiscard]] DeferredLiteral numeric_sub_deferred(DeferredLiteral const &lhs,
                                                   DeferredLiteral const &rhs,
                                                   storage::DynNodeStoragePtr node_storage = storage::default_node_storage);
[[nodiscard]] DeferredLiteral numeric_mul_deferred(DeferredLiteral const &lhs,
                                                   DeferredLiteral const &rhs,
                                                   storage::DynNodeStoragePtr node_storage = storage::default_node_storage);
[[nodiscard]] DeferredLiteral numeric_div_deferred(DeferredLiteral const &lhs,
                                                   DeferredLiteral const &rhs,
                                                   storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

struct NumericAddWithLossResult {
    DeferredLiteral result;
    DeferredLiteral loss;
};

/**
 * Adds lhs and rhs and reports what the addition lost to rounding.
 *
 * @return the sum and the loss, or two null-values if either side is not numeric or the two have no
 *      common numeric datatype. The loss alone is the null-value if the sum came out infinite,
 *      whose only loss would be inf - inf = NaN.
 */
[[nodiscard]] NumericAddWithLossResult numeric_add_with_loss_deferred(DeferredLiteral const &lhs,
                                                                      DeferredLiteral const &rhs,
                                                                      storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

}  // namespace rdf4cpp

#endif // RDF4CPP_DEFERRED_LITERAL_HPP
