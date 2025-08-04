#ifndef RDF4CPP_REGISTRY_UTIL_INLINING_HPP
#define RDF4CPP_REGISTRY_UTIL_INLINING_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::datatypes::registry::util {

namespace packing_detail {

    template<typename T>
    struct ReprType {
        using type = T;
    };

    template<typename T> requires requires { typename T::underlying_type; }
    struct ReprType<T> {
        using type = typename T::underlying_type;
    };

    template<typename T>
    using ReprType_t = typename ReprType<T>::type;

    template<typename T>
    constexpr ReprType_t<T> to_underlying(T value) {
        if constexpr (std::is_same_v<ReprType_t<T>, T>) {
            return value;
        } else {
            return value.to_underlying();
        }
    }

    template<typename P, typename T>
    union __attribute__((__packed__)) PackingHelper {
        static_assert(sizeof(P) >= sizeof(T), "Packed into type must be at least as large as type to pack");
        static constexpr size_t value_padding = sizeof(P) - sizeof(T);

        struct __attribute__((__packed__)) {
            T value;
            std::array<std::byte, value_padding> pad{};
        } unpacked_value;

        P packed_value;
    };

    /**
     * @brief determines if the bit at position `bit` has the same value as every other higher bit after it
     * @tparam bit last bit that is supposed to carry information
     * @param value value to check
     */
    template<size_t bit, typename T>
    constexpr bool no_information_in_bits_after(T value) noexcept {
        if constexpr (bit >= sizeof(T) * 8) {
            return true;
        } else {
            auto const indicator_bit = (value >> bit) & 1;
            auto const mask = ~((indicator_bit << (bit + 1)) - 1);

            return value == (value | mask);
        }
    }

} // namespace packing_detail

/**
 * @brief packs the bits of a value into the lower bits of a bigger type
 * @tparam P bigger type to pack the bits into
 * @tparam T smaller type to be packed
 * @param value value to be packed
 * @return the packed value
 * @note bits that do not fit into a P are discarded
 */
template<typename P, typename T>
constexpr P pack(T value) noexcept {
    using RP = packing_detail::ReprType_t<P>;

    packing_detail::PackingHelper<RP, T> reinterpret{.unpacked_value = {.value = value}};
    return P{reinterpret.packed_value};
}

/**
 * @brief reverse operation of pack
 * @tparam T smaller type to be unpacked
 * @tparam P bigger type that the value of T was packed into
 * @param packed_value the packed value to unpack
 * @return the unpacked value
 * @note bits that do not fit into a T are discarded
 */
template<typename T, typename P>
constexpr T unpack(P packed_value) noexcept {
    using RP = packing_detail::ReprType_t<P>;

    packing_detail::PackingHelper<RP, T> reinterpret{.packed_value = packing_detail::to_underlying(packed_value)};
    return reinterpret.unpacked_value.value;
}

/**
 * @brief packs the value of a signed integral type into the lower `bits` bits of P
 * @tparam P type to pack into
 * @tparam bits bits available in P for packing
 * @tparam T to be packed type
 * @param value to be packed value
 * @return packed value
 */
template<typename P, size_t bits, std::signed_integral T>
constexpr P pack_signed(T value) noexcept {
    // bit at position bits - 1 must have the same value as every bit to the left of it
    RDF4CPP_ASSERT(packing_detail::no_information_in_bits_after<bits - 1>(value));

    if constexpr (sizeof(T) * 8 > bits) {
        using UT = std::make_unsigned_t<T>;

        // clear bits without information if necessary
        UT const keep_mask = (UT{1} << bits) - 1;
        value &= keep_mask;
    }

    return pack<P>(value);
}

/**
 * @brief reverse operation of pack_signed
 */
template<std::signed_integral T, size_t bits, typename P>
constexpr T unpack_signed(P packed_value) noexcept {
    auto const sign_ext_shift_amt = sizeof(T) * 8 - bits;

    // smear bit that indicates remaining/truncated bits to the left
    // note: depends on arithmetic right shift (implementation defined)
    return unpack<T>(packed_value) << sign_ext_shift_amt >> sign_ext_shift_amt;
}

/**
 * @brief tries to pack any integral value into the lower `bits` bits of a value of type P
 * @tparam P type to pack into
 * @tparam bits bits available in P for packing
 * @tparam T to be packed type
 * @param value to be packed value
 * @return the packed value if there was enough space to pack it, otherwise nullopt
 */
template<typename P, size_t bits, std::integral T>
constexpr std::optional<P> try_pack_integral(T value) noexcept {
    if constexpr (sizeof(T) * 8 <= bits) {
        return pack<P>(value);
    } else if constexpr (std::unsigned_integral<T>) {
        if (value >= (T{1} << bits)) [[unlikely]] {
            return std::nullopt;
        }

        return pack<P>(value);
    } else {
        constexpr auto boundary = T{1} << (bits - 1);

        if (value >= boundary || value < -boundary) [[unlikely]] {
            return std::nullopt;
        }

        return pack_signed<P, bits>(value);
    }
}

template<typename P, std::integral T>
constexpr std::optional<P> try_pack_integral(T value) noexcept {
    return try_pack_integral<P, P::width>(value);
}

/**
 * @brief reverse operation of pack_integral
 */
template<std::integral T, size_t bits, typename P>
constexpr T unpack_integral(P packed_value) noexcept {
    if constexpr (sizeof(T) * 8 <= bits || std::unsigned_integral<T>) {
        return unpack<T>(packed_value);
    } else {
        return unpack_signed<T, bits>(packed_value);
    }
}

template<std::integral T, typename P>
constexpr T unpack_integral(P packed_value) noexcept {
    return unpack_integral<T, P::width>(packed_value);
}

} // namespace rdf4cpp::datatypes::registry::util

#endif  //RDF4CPP_REGISTRY_UTIL_INLINING_HPP
