#ifndef RDF4CPP_INT128_HPP
#define RDF4CPP_INT128_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>
#include <dice/hash.hpp>

namespace rdf4cpp::util {
#ifdef __SIZEOF_INT128__
    using Int128 = __int128;
    using UInt128 = unsigned __int128;
#else
    using Int128 = boost::multiprecision::checked_int128_t;
    using UInt128 = boost::multiprecision::checked_uint128_t;
#endif

    // string -> int128 can be found in CharConvExt

#ifdef __SIZEOF_INT128__
    inline bool to_chars_canonical(__int128 value, writer::BufWriterParts const writer) noexcept {
        if (value == 0) {
            return writer::write_str("0", writer);
        }
        // +1 because of definition of digits10 https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
        // +1 for sign
        static constexpr size_t buf_sz = std::numeric_limits<__int128>::digits10 + 1 + 1;

        std::array<char, buf_sz> buf;

        bool const neg = value < 0;
        char *curr = buf.data() + buf.size();
        while (value != 0) {
            auto c = static_cast<int>(value % 10);
            value = value / 10;
            if (neg) {
                c = -c;
            }
            --curr;
            assert(curr >= buf.data());
            *curr = static_cast<char>('0' + c);
        }
        if (neg) {
            --curr;
            assert(curr >= buf.data());
            *curr = '-';
        }

        return writer::write_str(std::string_view(curr, buf.data() + buf.size()), writer);
    }
#endif

    namespace detail {
        enum struct OverflowMode {
            Checked,
            UndefinedBehavior,
        };

        template<std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        using cpp_int_checked = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, SignType, boost::multiprecision::checked, Alloc>>;
        template<std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        using cpp_int_unchecked = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, SignType, boost::multiprecision::unchecked, Alloc>>;

        template<OverflowMode m, typename T>
        requires std::integral<T> || std::same_as<T, __int128> || std::same_as<T, unsigned __int128>
        static constexpr bool add_checked(T const &a, T const &b, T &result) noexcept {
            if constexpr (m == OverflowMode::Checked) {
                return __builtin_add_overflow(a, b, &result);
            } else {
                result = a + b;
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool add_checked(cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &b,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                try {
                    result = a + b;
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                using Unc = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>;
                result = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>{Unc{a} + Unc{b}};
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool add_checked(cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &b,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                using C = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>;
                try {
                    result = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>{C{a} + C{b}};
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                result = a + b;
                return false;
            }
        }


        template<OverflowMode m, typename T>
        requires std::integral<T> || std::same_as<T, __int128> || std::same_as<T, unsigned __int128>
        static constexpr bool sub_checked(T const &a, T const &b, T &result) noexcept {
            if constexpr (m == OverflowMode::Checked) {
                return __builtin_sub_overflow(a, b, &result);
            } else {
                result = a - b;
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool sub_checked(cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &b,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                try {
                    result = a - b;
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                using Unc = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>;
                result = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>{Unc{a} - Unc{b}};
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool sub_checked(cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &b,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                using C = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>;
                try {
                    result = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>{C{a} - C{b}};
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                result = a - b;
                return false;
            }
        }


        template<OverflowMode m, typename T>
        requires std::integral<T> || std::same_as<T, __int128> || std::same_as<T, unsigned __int128>
        static constexpr bool mul_checked(T const &a, T const &b, T &result) noexcept {
            if constexpr (m == OverflowMode::Checked) {
                return __builtin_mul_overflow(a, b, &result);
            } else {
                result = a * b;
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool mul_checked(cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &b,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                try {
                    result = a * b;
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                using Unc = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>;
                result = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>{Unc{a} * Unc{b}};
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool mul_checked(cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &b,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                using C = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>;
                try {
                    result = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>{C{a} * C{b}};
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                result = a * b;
                return false;
            }
        }


        template<OverflowMode m, typename T>
        requires std::integral<T> || std::same_as<T, __int128> || std::same_as<T, unsigned __int128>
        static constexpr bool pow_checked(T const &a, unsigned int b, T &result) noexcept {
            T r = 1;
            bool over = false;
            for (unsigned int i = 0; i < b; ++i) {
                over |= mul_checked<m, T>(r, a, r);
            }
            result = r;
            return over;
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool pow_checked(cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          unsigned int b,
                                          cpp_int_checked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                try {
                    result = boost::multiprecision::pow(a, b);
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                using Unc = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>;
                result = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>{boost::multiprecision::pow(Unc{a}, b)};
                return false;
            }
        }
        template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc>
        static constexpr bool pow_checked(cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> const &a,
                                          unsigned int b,
                                          cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
            if (m == OverflowMode::Checked) {
                using C = cpp_int_checked<MinBits, MaxBits, SignType, Alloc>;
                try {
                    result = cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>{boost::multiprecision::pow(C{a}, b)};
                } catch (std::overflow_error const &) {
                    return true;
                } catch (std::range_error const &) {
                    return true;
                }
                return false;
            } else {
                result = boost::multiprecision::pow(a, b);
                return false;
            }
        }
    }  // namespace detail
}  // namespace rdf4cpp::util

#ifdef __SIZEOF_INT128__

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::util::Int128> {
    static size_t dice_hash(rdf4cpp::util::Int128 const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::bit_cast<std::array<int64_t, 2>>(x));
    }
};
#endif

#endif  //RDF4CPP_INT128_HPP
