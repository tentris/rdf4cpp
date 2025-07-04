#ifndef RDF4CPP_INT128_HPP
#define RDF4CPP_INT128_HPP

// checked arithmetic functions returning a boolean to indicate failure
// follow the example of https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// and return true, iff the operation failed.

#include <boost/multiprecision/cpp_int.hpp>
#include <dice/hash.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>

namespace rdf4cpp {
#ifdef __SIZEOF_INT128__
    using Int128 = __int128;
#else
    using Int128 = boost::multiprecision::checked_int128_t;
#endif

    namespace util {
        // string -> int128 can be found in CharConvExt

#ifdef __SIZEOF_INT128__
        inline std::to_chars_result to_chars(char *first, char *last, unsigned __int128 val) noexcept {
            static constexpr unsigned __int128 max_pow10 = []() {
                auto n = std::numeric_limits<uint64_t>::digits10;
                unsigned __int128 d = 1;
                for (int i = 0; i < n; ++i) {
                    d = d * 10;
                }
                return d;
            }();

            if (val <= std::numeric_limits<uint64_t>::max()) {
                return std::to_chars(first, last, static_cast<uint64_t>(val));
            }

            std::to_chars_result upper = to_chars(first, last, val / max_pow10);
            if (upper.ec != std::errc{}) {
                return upper;
            }

            first = upper.ptr;

            std::to_chars_result lower = std::to_chars(first, last, static_cast<uint64_t>(val % max_pow10));
            return lower;
        }
        inline std::to_chars_result to_chars(char *first, char *last, __int128 val) noexcept {
            if (val >= 0) {
                return to_chars(first, last, static_cast<unsigned __int128>(val));
            }

            if (val == std::numeric_limits<__int128>::min()) [[unlikely]] {
                static constexpr std::string_view data = "-170141183460469231731687303715884105728";
                static constexpr auto write = data;
                if ((last - first) < static_cast<int64_t>(write.size())) {
                    return std::to_chars_result{.ptr = first, .ec = std::errc::value_too_large};
                }
                std::memcpy(first, write.data(), write.size());
                return std::to_chars_result{.ptr = first + write.size(), .ec = std::errc{}};
            }

            if (first == last) [[unlikely]] {
                return std::to_chars_result{.ptr = first, .ec = std::errc::value_too_large};
            }

            *first++ = '-';
            return to_chars(first, last, static_cast<unsigned __int128>(-val));
        }
        inline bool to_chars_canonical(__int128 const value, writer::BufWriterParts const writer) noexcept {
            // +1 because of definition of digits10 https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
            // +1 for sign
            static constexpr size_t buf_sz = std::numeric_limits<__int128>::digits10 + 1 + 1;

            std::array<char, buf_sz> buf;
            std::to_chars_result const res = to_chars(buf.data(), buf.data() + buf.size(), value);

            assert(res.ec == std::errc{});

            std::string_view const s{buf.data(), static_cast<std::string::size_type>(res.ptr - buf.data())};
            return writer::write_str(s, writer);
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

            template<typename T>
            concept IntegralExt = std::integral<T> || std::same_as<T, __int128> || std::same_as<T, unsigned __int128>;
            template<typename T>
            concept BoostNumber = std::same_as<T, boost::multiprecision::number<typename T::backend_type, T::et>>;

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

            template<OverflowMode m, typename T>
            requires IntegralExt<T>
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

            template<typename T>
            struct MakeUnsigned {
                using t = std::make_unsigned_t<T>;
            };
            template<>
            struct MakeUnsigned<__int128> {
                using t = unsigned __int128;
            };
            template<std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_int_check_type Checked, typename Alloc>
            struct MakeUnsigned<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, boost::multiprecision::signed_magnitude, Checked, Alloc>>> {
                using t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, boost::multiprecision::unsigned_magnitude, Checked, Alloc>>;
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
                        if (f < 0 || static_cast<typename MakeUnsigned<From>::t>(f) > std::numeric_limits<To>::max()) {
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
            template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc, typename To>
            static constexpr bool cast_checked(cpp_int_checked<MinBits, MaxBits, SignType, Alloc> const &f, To &result) noexcept {
                if constexpr (m == OverflowMode::Checked) {
                    try {
                        result = static_cast<To>(f);
                    } catch (std::overflow_error const &) {
                        return true;
                    } catch (std::range_error const &) {
                        return true;
                    }
                    return false;
                }
                result = static_cast<To>(static_cast<cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>>(f));
                return false;
            }
            template<OverflowMode m, std::size_t MinBits, std::size_t MaxBits, boost::multiprecision::cpp_integer_type SignType, typename Alloc, IntegralExt From>
            static constexpr bool cast_checked(From const &f, cpp_int_checked<MinBits, MaxBits, SignType, Alloc> &result) noexcept {
                if constexpr (m == OverflowMode::Checked) {
                    try {
                        result = static_cast<cpp_int_checked<MinBits, MaxBits, SignType, Alloc>>(f);
                    } catch (std::overflow_error const &) {
                        return true;
                    } catch (std::range_error const &) {
                        return true;
                    }
                    return false;
                }
                result = static_cast<cpp_int_checked<MinBits, MaxBits, SignType, Alloc>>(static_cast<cpp_int_unchecked<MinBits, MaxBits, SignType, Alloc>>(f));
                return false;
            }
        }  // namespace detail

        template<detail::BoostNumber T>
        bool to_chars_canonical(T const &value, writer::BufWriterParts const writer) noexcept {
            std::stringstream s{};
            s << value;
            return writer::write_str(s.view(), writer);
        }
    }  // namespace util
}  // namespace rdf4cpp

#ifdef __SIZEOF_INT128__

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, __int128> {
    static size_t dice_hash(__int128 const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::bit_cast<std::array<int64_t, 2>>(x));
    }
};
template<typename Policy, typename N, boost::multiprecision::expression_template_option et>
struct dice::hash::dice_hash_overload<Policy, boost::multiprecision::number<N, et>> {
    static size_t dice_hash(boost::multiprecision::number<N, et> const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(::boost::multiprecision::hash_value(x));
    }
};
inline std::ostream &operator<<(std::ostream &str, __int128 const &bn) {
    rdf4cpp::writer::BufOStreamWriter w{str};
    rdf4cpp::util::to_chars_canonical(bn, w);
    w.finalize();
    return str;
}
#endif

#endif  //RDF4CPP_INT128_HPP
