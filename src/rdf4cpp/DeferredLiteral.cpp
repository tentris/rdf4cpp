#include <rdf4cpp/DeferredLiteral.hpp>

namespace rdf4cpp {

namespace deferred_detail {
DeferredLiteral make_deferred_from_value(std::any value,
                                       datatypes::registry::DatatypeIDView datatype,
                                       storage::DynNodeStoragePtr node_storage) {
    return DeferredLiteral{std::move(value), IRI{datatype, node_storage}};
}
}  // namespace deferred_detail

DeferredLiteral make_deferred_from_literal(Literal const &lit) {
    if (lit.null()) {
        return DeferredLiteral{};
    }

    return DeferredLiteral{lit.value(), lit.datatype()};
}

Literal materialize_deferred(DeferredLiteral value, storage::DynNodeStoragePtr node_storage) {
    return Literal::make_typed_from_value(std::move(value.value), value.datatype, node_storage);
}

DeferredLiteral numeric_add_deferred(DeferredLiteral const &lhs, DeferredLiteral const &rhs, storage::DynNodeStoragePtr node_storage) {
    return deferred_detail::numeric_binop_deferred_impl(
            [](auto const &num_ops) noexcept {
                return num_ops.add_fptr;
            },
            lhs, rhs, node_storage);
}

DeferredLiteral numeric_sub_deferred(DeferredLiteral const &lhs, DeferredLiteral const &rhs, storage::DynNodeStoragePtr node_storage) {
    return deferred_detail::numeric_binop_deferred_impl(
            [](auto const &num_ops) noexcept {
                return num_ops.sub_fptr;
            },
            lhs, rhs, node_storage);
}

DeferredLiteral numeric_mul_deferred(DeferredLiteral const &lhs, DeferredLiteral const &rhs, storage::DynNodeStoragePtr node_storage) {
    return deferred_detail::numeric_binop_deferred_impl(
            [](auto const &num_ops) noexcept {
                return num_ops.mul_fptr;
            },
            lhs, rhs, node_storage);
}

DeferredLiteral numeric_div_deferred(DeferredLiteral const &lhs, DeferredLiteral const &rhs, storage::DynNodeStoragePtr node_storage) {
    return deferred_detail::numeric_binop_deferred_impl(
            [](auto const &num_ops) noexcept {
                return num_ops.div_fptr;
            },
            lhs, rhs, node_storage);
}

// Fast2Sum (Dekker): for |a| >= |b|, (a - (a + b)) + b is exactly the rounding error of a + b.
// Adding those errors back up is what makes a running total compensated (Kahan-Babuska-Neumaier
// summation, see CompensatedSum). The exactness only holds within one floating point format, which
// is why the transformation runs on the equalized values rather than on the two DeferredLiterals.
// See https://en.wikipedia.org/wiki/2Sum
NumericAddWithLossResult numeric_add_with_loss_deferred(DeferredLiteral const &lhs,
                                                        DeferredLiteral const &rhs,
                                                        storage::DynNodeStoragePtr node_storage) {
    auto const op = [&](datatypes::registry::DatatypeRegistry::NumericOpsImpl const &num_ops,
                        std::any const &lhs,
                        std::any const &rhs) {
        auto sum = num_ops.add_fptr(lhs, rhs);
        if (!sum.result_value.has_value()) {
            return NumericAddWithLossResult{};
        }

        // neither add nor sub changes the datatype, so these same ops apply to the sum
        IRI const datatype{sum.result_type_id, node_storage};

        auto loss = [&]() -> DeferredLiteral {
            if (num_ops.is_inf_fptr(*sum.result_value)) {
                return {};  // an infinite sum has nothing to report but inf - inf = NaN
            }

            // Fast2Sum wants the operand of larger magnitude first. Operands that do not order
            // (NaN, or a datatype that cannot compare) may go either way: the loss is then merely
            // inexact, never wrong, because it is added to the total rather than replacing it
            bool const lhs_is_larger = num_ops.magnitude_compare_fptr != nullptr && std::is_gteq(num_ops.magnitude_compare_fptr(lhs, rhs));

            auto const diff = num_ops.sub_fptr(lhs_is_larger ? lhs : rhs, *sum.result_value);
            if (!diff.result_value.has_value()) {
                return {};
            }

            auto res = num_ops.add_fptr(*diff.result_value, lhs_is_larger ? rhs : lhs);
            if (!res.result_value.has_value()) {
                return {};
            }

            return DeferredLiteral{std::move(*res.result_value), datatype};
        }();

        return NumericAddWithLossResult{.result = DeferredLiteral{std::move(*sum.result_value), datatype}, .loss = std::move(loss)};
    };

    return deferred_detail::numeric_equalized_deferred_impl(op, NumericAddWithLossResult{}, lhs, rhs);
}

} // namespace rdf4cpp
