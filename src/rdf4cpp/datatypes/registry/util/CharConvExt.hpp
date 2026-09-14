#ifndef RDF4CPP_REGISTRY_CHARCONVEXT_HPP
#define RDF4CPP_REGISTRY_CHARCONVEXT_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <format>

// using boost::charconv instead of std::charconv
// because boost::charconv has an overload for __int128,
// whereas std::charconv only has that since libstdc++-16
#include <boost/charconv.hpp>
#include <boost/version.hpp>

#include <rdf4cpp/Int128.hpp>
#include <rdf4cpp/Assert.hpp>
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>
#include <rdf4cpp/datatypes/registry/util/ConstexprString.hpp>

namespace rdf4cpp::datatypes::registry::util {
/**
 * Serializes an integral type into its (SPARQL) representation.
 * For integers the canonical and simplified representation are identical.
 * see https://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#dt-integer
 *
 * @param value the value to be serialized
 * @param writer writer parts to be used for serialization
 * @return true if serialization successful, otherwise false
 */
template<IntegralExt I>
bool to_chars_canonical(I const value, writer::BufWriterParts const writer) noexcept {
    // +1 because of definition of digits10 https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
    // +1 for sign
    static_assert(std::numeric_limits<I>::is_specialized);
    static constexpr size_t buf_sz = std::numeric_limits<I>::digits10 + 1 + static_cast<size_t>(std::numeric_limits<I>::is_signed);

    std::array<char, buf_sz> buf;
    boost::charconv::to_chars_result const res = [&] {
        if constexpr (std::is_same_v<I, Int128> && BOOST_VERSION < 109100) {
            // boost < 1.91 has a bug that causes incorrect formatting for __int128 (aka rdf4cpp::Int128)
            char *first = buf.data();
            UInt128 const mag = value < 0 ? -static_cast<UInt128>(value) : static_cast<UInt128>(value);
            if (value < 0) {
                *first++ = '-';
            }
            return boost::charconv::to_chars(first, buf.data() + buf.size(), mag);
        } else {
            return boost::charconv::to_chars(buf.data(), buf.data() + buf.size(), value);
        }
    }();
    RDF4CPP_ASSERT(res.ec == std::errc{});

    std::string_view const s{buf.data(), static_cast<std::string::size_type>(res.ptr - buf.data())};
    return writer::write_str(s, writer);
}

/**
 * Parses a valid string representation of a floating point or integral number
 *
 * @tparam F the result floating point type
 * @param s the string to be parsed
 * @return the resulting value
 * @throws rdf4cpp::InvalidNode if the string cannot be parsed
 */
template<typename F, ConstexprString datatype>
requires (std::floating_point<F> || IntegralExt<F>)
F from_chars(std::string_view s) {
    if (s.starts_with('+')) {
        // from_chars does not allow initial +
        s.remove_prefix(1);
    }

    F value;
    boost::charconv::from_chars_result res = [&]() noexcept {
        if constexpr (std::floating_point<F>) {
            return boost::charconv::from_chars(s.data(), s.data() + s.size(), value, boost::charconv::chars_format::general);
        } else {
            return boost::charconv::from_chars(s.data(), s.data() + s.size(), value);
        }
    }();

    if (res.ec != std::errc{}) {
        if (res.ec == std::errc::invalid_argument) {
            // nothing parsed (res.ptr at end, e.g. "" or "+") or only a sign, otherwise fall through to the data check
            if (res.ptr == s.data() + s.size() || (std::numeric_limits<F>::is_signed && *res.ptr == '-')) {
                throw rdf4cpp::InvalidNode{std::format("{} parsing error: literal is empty", datatype)};
            }
        } else if (res.ec == std::errc::result_out_of_range) {
            throw rdf4cpp::InvalidNode{std::format("{} parsing error: {} is out of range", datatype, s)};
        } else {
            throw rdf4cpp::InvalidNode{std::format("{} parsing error: {}", datatype, std::make_error_code(res.ec).message())};
        }
    }
    if(res.ptr != s.data() + s.size()) {
        // parsing did not reach end of string => it contains invalid characters
        throw rdf4cpp::InvalidNode{std::format("{} parsing error: found {}, invalid for datatype", datatype, *res.ptr)};
    }

    return value;
}

namespace detail  {
/**
 * equivalent to static_cast<size_t>(1 + std::log10(value))
 * only exists because the above is not a constexpr in clang
 */
template<typename T>
constexpr size_t log10ceil(T const value) noexcept {
    if (value < 10) {
        return 1;
    }
    return 1 + log10ceil(value / 10);
}
} // namespace detail

/**
 * Serializes a floating point number into its (SPARQL) _canonical_ string representation.
 * This is always scientific notation.
 * see https://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#dt-float
 *
 * @param value the value to be serialized
 * @param writer writer parts to be used for serialization
 * @return true if serialization successful, otherwise false
 */
template<std::floating_point F>
bool to_chars_canonical(F const value, writer::BufWriterParts const writer) noexcept {
    if (std::isnan(value)) {
        return writer::write_str("NaN", writer);
    }

    if (std::isinf(value)) {
        if (value > 0) {
            return writer::write_str("INF", writer);
        } else {
            return writer::write_str("-INF", writer);
        }
    }

    // +1 for minus in mantissa
    // +1 for integral part
    // +1 for dot
    // +1 for E
    // +1 for minus in exponent
    // at least 2 for exponent because the (c++) standard says so (https://en.cppreference.com/w/cpp/utility/to_chars)
    static_assert(std::numeric_limits<F>::is_specialized);
    static constexpr size_t buf_sz = 5 + std::numeric_limits<F>::max_digits10 + std::max(2ul, detail::log10ceil(std::numeric_limits<F>::max_exponent10));
    std::array<char, buf_sz> buf;

    boost::charconv::to_chars_result res = boost::charconv::to_chars(buf.data(), buf.data() + buf.size(), value, boost::charconv::chars_format::scientific);
    RDF4CPP_ASSERT(res.ec == std::errc{});

    auto *e_ptr = std::find(buf.data(), res.ptr, 'e');
    RDF4CPP_ASSERT(e_ptr != res.ptr); // serializing in scientific notation, there must be an 'e'
    *e_ptr = 'E'; // convert 'e' to 'E' as required by the SPARQL standard

    if (auto *dot_ptr = buf.data() + 1; dot_ptr == e_ptr) {
        // mantissa is in integer format therefore missing '.0' after mantissa

        // make space for '.0' by shifting exponent right
        std::shift_right(e_ptr, buf.data() + buf.size(), 2);

        // write '.0'
        dot_ptr[0] = '.';
        dot_ptr[1] = '0';

        // adjust reference pointers
        e_ptr += 2;
        res.ptr += 2;
    }

    {
        // remove potential leading zeros and '+' in exponent
        std::string_view const exponent{e_ptr + 2, res.ptr};

        auto const shift_amt = [&]() {
            if (auto const non_zero_pos = exponent.find_first_not_of('0'); non_zero_pos != std::string::npos) {
                return non_zero_pos;
            }

            // exponent is only zeroes, keep 1 zero
            return exponent.size() - 1;
        }();

        auto const shift_off = static_cast<std::string::size_type>(e_ptr[1] == '+'); // need to shift potential '+' out as well
        res.ptr = std::shift_left(e_ptr + 1 + (1 - shift_off), res.ptr, shift_amt + shift_off); // shift out all leading zeros and plus sign from exponent
    }

    std::string_view const s{buf.data(), static_cast<std::string::size_type>(res.ptr - buf.data())};
    return writer::write_str(s, writer);
}

/**
 * Serializes a floating point number into its _simplified_ string representation which is e.g. used in
 * casting floats to strings.
 * see https://www.w3.org/TR/xpath-functions/#casting-to-string
 *
 * @param value the value to be serialized
 * @param writer writer parts to be used for serialization
 * @return true if serialization successful, otherwise false
 */
template<std::floating_point F>
bool to_chars_simplified(F const value, writer::BufWriterParts const writer) noexcept {
    if (value == 0) {
        return std::signbit(value) ? writer::write_str("-0", writer) : writer::write_str("0", writer);
    }

    if (auto const abs = std::abs(value); abs >= 0.000001 && abs < 1000000) {
        static_assert(std::numeric_limits<F>::is_specialized);
        static constexpr size_t buf_sz = 2 + std::numeric_limits<F>::max_exponent10 + std::numeric_limits<F>::max_digits10;
        std::array<char, buf_sz> buf;

        boost::charconv::to_chars_result const res = boost::charconv::to_chars(buf.data(), buf.data() + buf.size(), value, boost::charconv::chars_format::fixed);
        RDF4CPP_ASSERT(res.ec == std::errc{});

        std::string_view const s{buf.data(), static_cast<std::string::size_type>(res.ptr - buf.data())};
        return writer::write_str(s, writer);
    }

    return to_chars_canonical(value, writer);
}

} // namespace rdf4cpp::datatypes::registry::util

#endif  //RDF4CPP_REGISTRY_CHARCONVEXT_HPP
