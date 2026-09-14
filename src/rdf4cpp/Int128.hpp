#ifndef RDF4CPP_INT128_HPP
#define RDF4CPP_INT128_HPP

#include <concepts>
#include <limits>

// checked arithmetic functions returning a boolean to indicate failure
// follow the example of https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// and return true, iff the operation failed.

namespace rdf4cpp {
    using Int128 = __int128;
    using UInt128 = unsigned __int128;

    template<typename T>
    concept IntegralExt = std::integral<T> || std::same_as<T, Int128> || std::same_as<T, UInt128>;
    template<typename T>
    concept SignedIntegralExt = std::signed_integral<T> || std::same_as<T, Int128>;
    template<typename T>
    concept UnsignedIntegralExt = std::unsigned_integral<T> || std::same_as<T, UInt128>;

    namespace util {
        namespace detail {
            enum struct OverflowMode : bool {
                Checked,
                UndefinedBehavior,
            };

            template<OverflowMode m, typename T>
            requires IntegralExt<T>
            static constexpr bool add_checked(T const &a, T const &b, T &result) noexcept {
                if constexpr (m == OverflowMode::Checked) {
                    return __builtin_add_overflow(a, b, &result);
                } else {
                    result = a + b;
                    return false;
                }
            }

            template<OverflowMode m, typename T>
            requires IntegralExt<T>
            static constexpr bool sub_checked(T const &a, T const &b, T &result) noexcept {
                if constexpr (m == OverflowMode::Checked) {
                    return __builtin_sub_overflow(a, b, &result);
                } else {
                    result = a - b;
                    return false;
                }
            }

            template<OverflowMode m, typename T>
            requires IntegralExt<T>
            static constexpr bool mul_checked(T const &a, T const &b, T &result) noexcept {
                if constexpr (m == OverflowMode::Checked) {
                    return __builtin_mul_overflow(a, b, &result);
                } else {
                    result = a * b;
                    return false;
                }
            }

            template<OverflowMode m, typename T>
            requires IntegralExt<T>
            static constexpr bool pow_checked(T const &a, unsigned int b, T &result) noexcept {
                T r = 1;
                for (unsigned int i = 0; i < b; ++i) {
                    if (mul_checked<m, T>(r, a, r))
                        return true;
                }
                result = r;
                return false;
            }

            template<typename T>
            struct MakeUnsigned {
                using t = std::make_unsigned_t<T>;
            };
            template<>
            struct MakeUnsigned<__int128> {
                using t = unsigned __int128;
            };

            template<OverflowMode m, typename To, typename From>
            requires IntegralExt<To> && IntegralExt<From>
            static constexpr bool cast_checked(From const &f, To &result) noexcept {
                if constexpr (m == OverflowMode::Checked) {
                    if constexpr (std::numeric_limits<To>::is_signed == std::numeric_limits<From>::is_signed) {
                        if (std::numeric_limits<To>::min() > f || f > std::numeric_limits<To>::max()) {
                            return true;
                        }
                    } else if constexpr (std::numeric_limits<From>::is_signed) {
                        if (f < 0 || static_cast<MakeUnsigned<From>::t>(f) > std::numeric_limits<To>::max()) {
                            return true;
                        }
                    } else {
                        if (f > std::numeric_limits<To>::max()) {
                            return true;
                        }
                    }
                }
                result = static_cast<To>(f);
                return false;
            }
        }  // namespace detail
    }  // namespace util
}  // namespace rdf4cpp

#endif  //RDF4CPP_INT128_HPP
