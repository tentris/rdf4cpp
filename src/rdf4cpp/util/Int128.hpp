#ifndef RDF4CPP_INT128_HPP
#define RDF4CPP_INT128_HPP

// checked arithmetic functions returning a boolean to indicate failure
// follow the example of https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// and return true, iff the operation failed.

#include <dice/hash.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>

namespace rdf4cpp {
    using Int128 = __int128;

    namespace util {
        // string -> int128 can be found in CharConvExt

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
            if (upper.ec != std::errc{}) {  // NOLINT(*-invalid-enum-default-initialization)
                return upper;
            }

            if (last - upper.ptr < std::numeric_limits<uint64_t>::digits10) {
                return {last, std::errc::value_too_large};
            }

            std::array<char, std::numeric_limits<uint64_t>::digits10 + 1> buff;  // NOLINT(*-pro-type-member-init)
            std::to_chars_result lower = std::to_chars(buff.data(), buff.data() + buff.size(), static_cast<uint64_t>(val % max_pow10));

            if (lower.ec != std::errc{}) {  // NOLINT(*-invalid-enum-default-initialization)
                return lower;
            }

            auto const written = lower.ptr - buff.data();
            auto const n_zeroes = std::numeric_limits<uint64_t>::digits10 - written;

            char *curr = upper.ptr;
            RDF4CPP_DEBUG_ASSERT(last - curr >= n_zeroes);
            std::memset(curr, '0', n_zeroes);
            curr += n_zeroes;
            RDF4CPP_DEBUG_ASSERT(last - curr >= written);
            std::memcpy(curr, buff.data(), written);
            curr += written;

            lower.ptr = curr;

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
                return std::to_chars_result{.ptr = first + write.size(), .ec = std::errc{}};  // NOLINT(*-invalid-enum-default-initialization)
            }

            if (first == last) [[unlikely]] {
                return std::to_chars_result{.ptr = first, .ec = std::errc::value_too_large};
            }

            *first++ = '-';
            return to_chars(first, last, static_cast<unsigned __int128>(-val));
        }
        inline bool to_chars_canonical(__int128 const value, writer::BufWriterParts const &writer) noexcept {
            // +1 because of definition of digits10 https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
            // +1 for sign
            static constexpr size_t buf_sz = std::numeric_limits<__int128>::digits10 + 1 + 1;

            std::array<char, buf_sz> buf;  // NOLINT(*-pro-type-member-init)
            std::to_chars_result const res = to_chars(buf.data(), buf.data() + buf.size(), value);

            RDF4CPP_ASSERT(res.ec == std::errc{});  // NOLINT(*-invalid-enum-default-initialization)

            std::string_view const s{buf.data(), static_cast<std::string::size_type>(res.ptr - buf.data())};
            return writer::write_str(s, writer);
        }

        namespace detail {
            enum struct OverflowMode : bool {
                Checked,
                UndefinedBehavior,
            };

            template<typename T>
            concept IntegralExt = std::integral<T> || std::same_as<T, __int128> || std::same_as<T, unsigned __int128>;

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
                bool over = false;
                for (unsigned int i = 0; i < b; ++i) {
                    over |= mul_checked<m, T>(r, a, r);
                }
                result = r;
                return over;
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

inline std::ostream &operator<<(std::ostream &str, __int128 const &bn) {
    rdf4cpp::writer::BufOStreamWriter w{str};
    rdf4cpp::util::to_chars_canonical(bn, w);
    w.finalize();
    return str;
}

#endif  //RDF4CPP_INT128_HPP
