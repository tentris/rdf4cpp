#ifndef RDF4CPP_LITERALDATATYPEIMPL_HPP
#define RDF4CPP_LITERALDATATYPEIMPL_HPP

#include <rdf4cpp/datatypes/LiteralDatatype.hpp>
#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/DatatypeRegistry.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/util/ConstexprString.hpp>
#include <rdf4cpp/datatypes/registry/util/Inlining.hpp>
#include <rdf4cpp/storage/identifier/LiteralID.hpp>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string_view>
#include <type_traits>

namespace rdf4cpp::datatypes::registry {

namespace capabilities {

namespace detail {

/**
 * static_assert would always trigger if it wasn't dependent on a template parameter.
 * With this helper template, it only triggers if the function is instantiated.
 */
template<typename>
using always_false = std::false_type;
template<typename T>
static constexpr bool always_false_v = always_false<T>::value;

template<typename OpResMapping, typename Fallback, typename enable = void>
struct SelectOpResult {
    using type = typename OpResMapping::cpp_type;
};

template<typename Fallback>
struct SelectOpResult<std::false_type, Fallback> {
    using type = Fallback;
};

}  // namespace detail

/**
 * The default capability. All LiteralDatatypes must implement these operations.
 */
template<util::ConstexprString type_iri_t>
struct Default {
    /**
     * IRI of the LiteralDatatype.
     */
    static constexpr util::ConstexprString identifier = type_iri_t;

    /**
     * The C++ type that this LiteralDatatype is mapped to.
     */
    using cpp_type = typename DatatypeMapping<identifier>::cpp_datatype;

    /**
     * Factory function that parses a string representing datatype_t and builds an instance of datatype_t
     * @return instance of datatype_t
     */
    inline static cpp_type from_string(std::string_view) {
        // If this implementation is used the user forgot to provide their own.
        static_assert(detail::always_false_v<cpp_type>, "'from_string' is not implemented for this type!");
    }
    /**
     * Returns string representation of a datatype_t.
     * @param value the value
     * @return <div>value</div>'s canonical string representation
     */
    inline static bool serialize_canonical_string(cpp_type const &value, writer::BufWriterParts const writer) noexcept {
        // If not further specified, to_canonical_string is instantiated via operator<<. If operator<< is not defined for cpp_type instantiation will fail.
        std::stringstream str_s;
        str_s << value;

        return writer::write_str(str_s.view(), writer);
    }

    /**
     * Returns a string representation of a datatype that is intended to be shown in user friendly output.
     * E.g. when casting to xsd:string
     */
    inline static bool serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
        return serialize_canonical_string(value, writer);
    }
};


/**
 * The capability to be promoted to another LiteralDatatype, e.g. Decimal -> Float.
 */
template<util::ConstexprString type_iri>
struct Promotable {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    static constexpr size_t promotion_rank = detail_rank::DatatypePromotionRank<type_iri>::value;
    static constexpr size_t max_promotion_specialization_ix = DatatypePromotionSpecializationOverride<type_iri>::max_specialization_ix;

    template<size_t ix = 0>
    using promoted = typename detail_rank::NthPromotion<type_iri, ix>::promoted;

    template<size_t ix = 0>
    using promoted_cpp_type = typename DatatypeMapping<promoted<ix>::identifier>::cpp_datatype;

    template<size_t ix = 0>
    static nonstd::expected<promoted_cpp_type<ix>, DynamicError> promote(cpp_type const &value) noexcept {
        return static_cast<promoted_cpp_type<ix>>(value);
    }

    template<size_t ix = 0>
    static nonstd::expected<cpp_type, DynamicError> demote(promoted_cpp_type<ix> const &value) noexcept {
        if constexpr (std::is_integral_v<cpp_type> && std::is_integral_v<promoted_cpp_type<ix>>) {
            if (!std::in_range<cpp_type>(value)) {
                return nonstd::make_unexpected(DynamicError::InvalidValueForCast);
            }
        }

        return static_cast<cpp_type>(value);
    }
};

/**
 * The capability to be converted to it's supertype, e.g. Int -> Integer.
 */
template<util::ConstexprString type_iri>
struct Subtype {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    static constexpr size_t subtype_rank = detail_rank::DatatypeSubtypeRank<type_iri>::value;
    static constexpr size_t max_supertype_specialization_ix = DatatypeSupertypeSpecializationOverride<type_iri>::max_specialization_ix;

    template<size_t ix = 0>
    using supertype = typename detail_rank::NthSupertype<type_iri, ix>::supertype;

    template<size_t ix = 0>
    using super_cpp_type = typename DatatypeMapping<supertype<ix>::identifier>::cpp_datatype;

    template<size_t ix = 0>
    inline static super_cpp_type<ix> into_supertype(cpp_type const &value) noexcept {
        return static_cast<super_cpp_type<ix>>(value);
    }

    template<size_t ix = 0>
    inline static nonstd::expected<cpp_type, DynamicError> from_supertype(super_cpp_type<ix> const &value) noexcept {
        if constexpr (std::is_integral_v<cpp_type> && std::is_integral_v<super_cpp_type<ix>>) {
            if (!std::in_range<cpp_type>(value)) {
                return nonstd::make_unexpected(DynamicError::InvalidValueForCast);
            }
        }

        return static_cast<cpp_type>(value);
    }
};

/**
 * The capability to be used in numeric operations.
 */
template<util::ConstexprString type_iri>
struct Numeric {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    using add_result = typename DatatypeAddResultMapping<type_iri>::op_result;
    using sub_result = typename DatatypeSubResultMapping<type_iri>::op_result;
    using mul_result = typename DatatypeMulResultMapping<type_iri>::op_result;
    using div_result = typename DatatypeDivResultMapping<type_iri>::op_result;
    using pos_result = typename DatatypePosResultMapping<type_iri>::op_result;
    using neg_result = typename DatatypeNegResultMapping<type_iri>::op_result;
    using abs_result = typename DatatypeAbsResultMapping<type_iri>::op_result;
    using round_result = typename DatatypeRoundResultMapping<type_iri>::op_result;
    using floor_result = typename DatatypeFloorResultMapping<type_iri>::op_result;
    using ceil_result  = typename DatatypeCeilResultMapping<type_iri>::op_result;

    static_assert(LiteralDatatypeOrUndefined<add_result>, "add result must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<sub_result>, "sub result must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<mul_result>, "mul result must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<div_result>, "div result must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<pos_result>, "pos result must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<neg_result>, "neg result must be a literal datatype");

    using add_result_cpp_type = typename detail::SelectOpResult<add_result, cpp_type>::type;
    using sub_result_cpp_type = typename detail::SelectOpResult<sub_result, cpp_type>::type;
    using mul_result_cpp_type = typename detail::SelectOpResult<mul_result, cpp_type>::type;
    using div_result_cpp_type = typename detail::SelectOpResult<div_result, cpp_type>::type;
    using pos_result_cpp_type = typename detail::SelectOpResult<pos_result, cpp_type>::type;
    using neg_result_cpp_type = typename detail::SelectOpResult<neg_result, cpp_type>::type;
    using abs_result_cpp_type = typename detail::SelectOpResult<abs_result, cpp_type>::type;
    using round_result_cpp_type = typename detail::SelectOpResult<round_result, cpp_type>::type;
    using floor_result_cpp_type = typename detail::SelectOpResult<floor_result, cpp_type>::type;
    using ceil_result_cpp_type = typename detail::SelectOpResult<ceil_result, cpp_type>::type;

    inline static cpp_type zero_value() noexcept {
        return cpp_type{0};
    }

    inline static cpp_type one_value() noexcept {
        return cpp_type{1};
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-add
    inline static nonstd::expected<add_result_cpp_type, DynamicError> add(cpp_type const &lhs, cpp_type const &rhs) noexcept {
        if constexpr (std::is_integral_v<cpp_type>) {
            cpp_type res;
            bool const overflowed = __builtin_add_overflow(lhs, rhs, &res);

            if (overflowed) {
                return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
            }

            return res;
        } else {
            return lhs + rhs;
        }
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-subtract
    inline static nonstd::expected<sub_result_cpp_type, DynamicError> sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
        if constexpr (std::is_integral_v<cpp_type>) {
            cpp_type res;
            bool const overflowed = __builtin_sub_overflow(lhs, rhs, &res);

            if (overflowed) {
                return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
            }

            return res;
        } else {
            return lhs - rhs;
        }
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-multiply
    inline static nonstd::expected<mul_result_cpp_type, DynamicError> mul(cpp_type const &lhs, cpp_type const &rhs) noexcept {
        if constexpr (std::is_integral_v<cpp_type>) {
            cpp_type res;
            bool const overflowed = __builtin_mul_overflow(lhs, rhs, &res);

            if (overflowed) {
                return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
            }

            return res;
        } else {
            return lhs * rhs;
        }
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-divide
    inline static nonstd::expected<div_result_cpp_type, DynamicError> div(cpp_type const &lhs, cpp_type const &rhs) noexcept {
        if constexpr (std::is_integral_v<cpp_type>) {
            if (rhs == 0) {
                return nonstd::make_unexpected(DynamicError::DivideByZero);
            }
        }

        return lhs / rhs;
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-unary-plus
    inline static nonstd::expected<pos_result_cpp_type, DynamicError> pos(cpp_type const &operand) noexcept {
        return +operand;
    }

    // https://www.w3.org/TR/xpath-functions/#func-numeric-unary-minus
    inline static nonstd::expected<neg_result_cpp_type, DynamicError> neg(cpp_type const &operand) noexcept {
        if constexpr (std::is_unsigned_v<cpp_type>) {
            return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
        } else {
            return -operand;
        }
    }

    // https://www.w3.org/TR/xpath-functions/#func-abs
    inline static nonstd::expected<abs_result_cpp_type, DynamicError> abs([[maybe_unused]] cpp_type const &operand) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    // https://www.w3.org/TR/xpath-functions/#func-round
    inline static nonstd::expected<round_result_cpp_type, DynamicError> round([[maybe_unused]] cpp_type const &operand) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    // https://www.w3.org/TR/xpath-functions/#func-floor
    inline static nonstd::expected<floor_result_cpp_type, DynamicError> floor([[maybe_unused]] cpp_type const &operand) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    // https://www.w3.org/TR/xpath-functions/#func-ceiling
    inline static nonstd::expected<ceil_result_cpp_type, DynamicError> ceil([[maybe_unused]] cpp_type const &operand) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
};

/**
 * The capability to be used in numeric operations.
 * Except that this type itself does not implement the operations
 * and instead delegates to another type that does.
 */
template<util::ConstexprString type_iri>
struct NumericStub {
    using numeric_impl_type = typename DatatypeNumericStubMapping<type_iri>::numeric_impl_type;
    static_assert(NumericImplLiteralDatatype<numeric_impl_type>, "numeric_impl_type must actually be impl-numeric");
};


template<util::ConstexprString type_iri>
struct Timepoint {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    using timepoint_sub_result = typename DatatypeTimepointSubResultMapping<type_iri>::op_result;
    using timepoint_duration_operand_type = typename DatatypeTimepointDurationOperandMapping<type_iri>::duration_type;

    static_assert(LiteralDatatypeOrUndefined<timepoint_sub_result>, "timepoint sub result type must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<timepoint_duration_operand_type>, "timepoint duration operand type must be a literal datatype");

    using timepoint_sub_result_cpp_type = typename detail::SelectOpResult<timepoint_sub_result, cpp_type>::type;
    using timepoint_duration_operand_cpp_type = typename detail::SelectOpResult<timepoint_duration_operand_type, cpp_type>::type;

    static nonstd::expected<timepoint_sub_result_cpp_type, DynamicError> timepoint_sub([[maybe_unused]] cpp_type const &lhs, [[maybe_unused]] cpp_type const &rhs) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    static nonstd::expected<cpp_type, DynamicError> timepoint_duration_add([[maybe_unused]] cpp_type const &tp, [[maybe_unused]] timepoint_duration_operand_cpp_type const &dur) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    static nonstd::expected<cpp_type, DynamicError> timepoint_duration_sub([[maybe_unused]] cpp_type const &tp, [[maybe_unused]] timepoint_duration_operand_cpp_type const &dur) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
};

template<util::ConstexprString type_iri>
struct Duration {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    using duration_scalar_type = typename DatatypeDurationScalarMapping<type_iri>::scalar_type;
    using duration_div_result_type = typename DatatypeDurationDivResultMapping<type_iri>::op_result;

    static_assert(LiteralDatatypeOrUndefined<duration_scalar_type>, "duration scalar type must be a literal datatype");
    static_assert(LiteralDatatypeOrUndefined<duration_div_result_type>, "duration div result type must be a literal datatype");

    using duration_scalar_cpp_type = typename detail::SelectOpResult<duration_scalar_type, cpp_type>::type;
    using duration_div_result_cpp_type = typename detail::SelectOpResult<duration_div_result_type, cpp_type>::type;

    static nonstd::expected<cpp_type, DynamicError> duration_add([[maybe_unused]] cpp_type const &lhs, [[maybe_unused]] cpp_type const &rhs) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    static nonstd::expected<cpp_type, DynamicError> duration_sub([[maybe_unused]] cpp_type const &lhs, [[maybe_unused]] cpp_type const &rhs) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    static nonstd::expected<duration_div_result_cpp_type, DynamicError> duration_div([[maybe_unused]] cpp_type const &lhs, [[maybe_unused]] cpp_type const &rhs) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    static nonstd::expected<cpp_type, DynamicError> duration_scalar_mul([[maybe_unused]] cpp_type const &dur, [[maybe_unused]] duration_scalar_cpp_type const &scalar) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }

    static nonstd::expected<cpp_type, DynamicError> duration_scalar_div([[maybe_unused]] cpp_type const &dur, [[maybe_unused]] duration_scalar_cpp_type const &scalar) noexcept {
        return nonstd::make_unexpected(DynamicError::Unsupported);
    }
};

/**
 * The capability to be used in boolean contexts.
 */
template<util::ConstexprString type_iri>
struct Logical {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    /**
     * @return the EBV (effective boolean value) of a datatype
     */
    inline static bool effective_boolean_value(cpp_type const &) noexcept {
        static_assert(detail::always_false_v<cpp_type>, "'effective_boolean_value' is not implemented for this type!");
        return false;  // supress warnings in gcc, because it cannot properly detect that this is unreachable
    }
};

/**
 * The capability to be compared, e.g. with operator<=>
 */
template<util::ConstexprString type_iri>
struct Comparable {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    inline static std::partial_ordering compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
        return lhs <=> rhs;
    }
};

/**
 * The capability to have a fixed id and therefore
 * be looked up in constant time
 */
template<util::ConstexprString type_iri>
struct FixedId {
    static constexpr LiteralType fixed_id = reserved_datatype_ids[type_iri];
    static_assert(fixed_id.is_fixed(), "cannot treat non fixed id as fixed");
};

/**
 * The capability for values to be inlined into the LiteralID part of the NodeID.
 * @note this capability requires the FixedId capability, for more details see static_assert in LiteralDatatypeImpl
 */
template<util::ConstexprString type_iri>
struct Inlineable {
    using cpp_type = typename DatatypeMapping<type_iri>::cpp_datatype;

    static constexpr std::true_type is_inlineable{};

    static std::optional<storage::identifier::LiteralID> try_into_inlined([[maybe_unused]] cpp_type const &value) noexcept {
        if constexpr (std::integral<cpp_type>) {
            return util::try_pack_integral<storage::identifier::LiteralID>(value);
        } else {
            static_assert(detail::always_false_v<cpp_type>, "to_inlined not implemented for inlineable type");
            return std::nullopt; // silence gcc no-return warning
        }
    }

    static cpp_type from_inlined([[maybe_unused]] storage::identifier::LiteralID inlined) noexcept {
        if constexpr (std::integral<cpp_type>) {
            return util::unpack_integral<cpp_type>(inlined);
        } else {
            static_assert(detail::always_false_v<cpp_type>, "from_inlined not implemented for inlineable type");
            return {}; // silence gcc no-return warning
        }
    }
};

} // namespace capabilities

/**
 * An automatically registering LiteralDatatype with given capabilities.
 *
 * @tparam Capabilities all capabilities this instantiation should have
 */
template<util::ConstexprString type_iri, template<util::ConstexprString> typename... Capabilities>
struct LiteralDatatypeImpl : capabilities::Default<type_iri>, Capabilities<type_iri>... {
    using typename capabilities::Default<type_iri>::cpp_type;

private:
    static constexpr auto get_datatype_id() {
        if constexpr (HasFixedId<LiteralDatatypeImpl>) {
            return DatatypeIDView{LiteralDatatypeImpl::fixed_id};
        } else {
            return DatatypeIDView{LiteralDatatypeImpl::identifier};
        }
    }
public:
    static constexpr DatatypeIDView const datatype_id = get_datatype_id();

    static_assert((datatype_id.is_dynamic() && !reserved_datatype_ids.contains(LiteralDatatypeImpl::identifier))
                          || (datatype_id.is_fixed() && reserved_datatype_ids.contains(LiteralDatatypeImpl::identifier)),
                  "Mismatch between declared and actual fixed id mapping state. "
                  "Hint: maybe you forgot declare the fixed id or to add the FixedId capability. "
                  "Note: this would cause inconsistency between registry and node storage");

    static_assert(!IsInlineable<LiteralDatatypeImpl> || datatype_id.is_fixed(),
                  "Inlineable datatypes are required to have a fixed id."
                  "Hint: either make this datatype not inlineable or add a fixed id."
                  "Note: inlineable datatypes with dynamic id would not be able to recover their IRI using the node storage handle.");
private:
    static std::nullptr_t init() noexcept {
        DatatypeRegistry::add<LiteralDatatypeImpl>();
        return nullptr;
    }

    [[maybe_unused]] std::nullptr_t const init_dummy = init();
};

}  // namespace rdf4cpp::datatypes::registry
#endif  //RDF4CPP_LITERALDATATYPEIMPL_HPP
