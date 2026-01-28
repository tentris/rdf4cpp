#ifndef RDF4CPP_BOOST_INT_HPP
#define RDF4CPP_BOOST_INT_HPP

// checked arithmetic functions returning a boolean to indicate failure
// follow the example of https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
// and return true, iff the operation failed.

#include <boost/multiprecision/cpp_int.hpp>
#include <rdf4cpp/util/Int128.hpp>


namespace rdf4cpp::util::detail {
    template<typename T>
    concept BoostNumber = std::same_as<T, boost::multiprecision::number<typename T::backend_type, T::et>>;

    template<std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc>
    using cpp_int_checked = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<min_bits, max_bits, sign_type, boost::multiprecision::checked, Alloc>>;
    template<std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc>
    using cpp_int_unchecked = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<min_bits, max_bits, sign_type, boost::multiprecision::unchecked, Alloc>>;


    template<OverflowMode m, std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc>
    static constexpr bool add_checked(cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &a,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &b,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> &result) noexcept {
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
            using Unc = cpp_int_unchecked<min_bits, max_bits, sign_type, Alloc>;
            result = cpp_int_checked<min_bits, max_bits, sign_type, Alloc>{Unc{a} + Unc{b}};
            return false;
        }
    }

    template<OverflowMode m, std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc>
    static constexpr bool sub_checked(cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &a,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &b,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> &result) noexcept {
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
            using Unc = cpp_int_unchecked<min_bits, max_bits, sign_type, Alloc>;
            result = cpp_int_checked<min_bits, max_bits, sign_type, Alloc>{Unc{a} - Unc{b}};
            return false;
        }
    }

    template<OverflowMode m, std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc>
    static constexpr bool mul_checked(cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &a,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &b,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> &result) noexcept {
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
            using Unc = cpp_int_unchecked<min_bits, max_bits, sign_type, Alloc>;
            result = cpp_int_checked<min_bits, max_bits, sign_type, Alloc>{Unc{a} * Unc{b}};
            return false;
        }
    }

    template<OverflowMode m, std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc>
    static constexpr bool pow_checked(cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &a,
                                      unsigned int b,
                                      cpp_int_checked<min_bits, max_bits, sign_type, Alloc> &result) noexcept {
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
            using Unc = cpp_int_unchecked<min_bits, max_bits, sign_type, Alloc>;
            result = cpp_int_checked<min_bits, max_bits, sign_type, Alloc>{boost::multiprecision::pow(Unc{a}, b)};
            return false;
        }
    }

    template<std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_int_check_type checked, typename Alloc>
    struct MakeUnsigned<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<min_bits, max_bits, boost::multiprecision::signed_magnitude, checked, Alloc>>> {
        using t = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<min_bits, max_bits, boost::multiprecision::unsigned_magnitude, checked, Alloc>>;
    };


    template<OverflowMode m, std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc, typename To>
    static constexpr bool cast_checked(cpp_int_checked<min_bits, max_bits, sign_type, Alloc> const &f, To &result) noexcept {
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
        result = static_cast<To>(static_cast<cpp_int_unchecked<min_bits, max_bits, sign_type, Alloc>>(f));
        return false;
    }
    template<OverflowMode m, std::size_t min_bits, std::size_t max_bits, boost::multiprecision::cpp_integer_type sign_type, typename Alloc, IntegralExt From>
    static constexpr bool cast_checked(From const &f, cpp_int_checked<min_bits, max_bits, sign_type, Alloc> &result) noexcept {
        if constexpr (m == OverflowMode::Checked) {
            try {
                result = static_cast<cpp_int_checked<min_bits, max_bits, sign_type, Alloc>>(f);
            } catch (std::overflow_error const &) {
                return true;
            } catch (std::range_error const &) {
                return true;
            }
            return false;
        }
        result = static_cast<cpp_int_checked<min_bits, max_bits, sign_type, Alloc>>(static_cast<cpp_int_unchecked<min_bits, max_bits, sign_type, Alloc>>(f));
        return false;
    }
}  // namespace rdf4cpp::util::detail

namespace rdf4cpp::util {
    template<detail::BoostNumber T>
    bool to_chars_canonical(T const &value, writer::BufWriterParts const &writer) noexcept {
        std::stringstream s{};
        s << value;
        return writer::write_str(s.view(), writer);
    }
}  // namespace rdf4cpp::util


template<typename Policy, typename N, boost::multiprecision::expression_template_option et>
struct dice::hash::dice_hash_overload<Policy, boost::multiprecision::number<N, et>> {
    static size_t dice_hash(boost::multiprecision::number<N, et> const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(::boost::multiprecision::hash_value(x));
    }
};

#endif  //RDF4CPP_BOOST_INT_HPP
