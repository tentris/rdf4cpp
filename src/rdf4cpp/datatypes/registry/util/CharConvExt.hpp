#ifndef RDF4CPP_REGISTRY_CHARCONVEXT_HPP
#define RDF4CPP_REGISTRY_CHARCONVEXT_HPP

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <concepts>
#include <stdexcept>
#include <format>

#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/Int128.hpp>

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
template<std::integral I>
bool to_chars_canonical(I const value, writer::BufWriterParts const writer) noexcept {
    // +1 because of definition of digits10 https://en.cppreference.com/w/cpp/types/numeric_limits/digits10
    // +1 for sign
    static constexpr size_t buf_sz = std::numeric_limits<I>::digits10 + 1 + static_cast<size_t>(std::is_signed_v<I>);

    std::array<char, buf_sz> buf;
    std::to_chars_result const res = std::to_chars(buf.data(), buf.data() + buf.size(), value);

    assert(res.ec == std::errc{});

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
requires (std::floating_point<F> || std::integral<F>)
F from_chars(std::string_view s) {
    if (s.starts_with('+')) {
        // from_chars does not allow initial +
        s.remove_prefix(1);
    }

    F value;
    std::from_chars_result res = [&]() noexcept {
        if constexpr (std::floating_point<F>) {
            return std::from_chars(s.data(), s.data() + s.size(), value, std::chars_format::general);
        } else {
            return std::from_chars(s.data(), s.data() + s.size(), value);
        }
    }();

    if (res.ec != std::errc{}) {
        if (res.ec == std::errc::invalid_argument) {
            bool neg = true;
            if constexpr(!std::unsigned_integral<F>) {
                neg = *res.ptr != '-';
            }

            if (res.ptr != s.data() + s.size() && neg) {
                // fallthrough to data check
            } else {
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

template<rdf4cpp::util::detail::BoostNumber F, ConstexprString datatype>
F from_chars(std::string_view s) {
    if (s.starts_with('+')) {
        s.remove_prefix(1);
    }
    if (auto const pos = s.find_first_not_of('0'); pos != std::string::npos) {
        s.remove_prefix(pos);
    }

    try {
        return F{s};
    }
    catch (std::runtime_error const &e) {
        throw InvalidNode{std::format("{} parsing error: {}", datatype, e.what())};
    }
}

#ifdef __SIZEOF_INT128__
template<typename F, ConstexprString datatype>
requires std::same_as<F, __int128>
F from_chars(std::string_view s) {
    static constexpr __int128 max_pow10 = []() {
        auto n = std::numeric_limits<uint64_t>::digits10;
        __int128 d = 1;
        for (int i = 0; i < n; ++i) {
            d = d * 10;
        }
        return d;
    }();

    if (s == "-170141183460469231731687303715884105728") {
        return std::numeric_limits<__int128>::min();
    }

    bool neg = false;
    if (s.starts_with('+')) {
        s.remove_prefix(1);
    }
    else if(s.starts_with('-')) {
        neg = true;
        s.remove_prefix(1);
    }

    __int128 value = 0;
    int i = 0;
    while (!s.empty()) {
        std::string_view p;
        if (s.size() <= std::numeric_limits<uint64_t>::digits10) {
            p = s;
            s = "";
        }
        else {
            const size_t pos = s.size() - std::numeric_limits<uint64_t>::digits10;
            p = s.substr(pos);
            s = s.substr(0, pos);
        }
        __int128 value2 = from_chars<uint64_t, datatype>(p);
        for (int j = 0; j < i; ++j) {
            if (rdf4cpp::util::detail::mul_checked<rdf4cpp::util::detail::OverflowMode::Checked>(value2, max_pow10, value2)) {
                throw rdf4cpp::InvalidNode{std::format("{} parsing error: overflow", datatype)};
            }
        }
        if (rdf4cpp::util::detail::add_checked<rdf4cpp::util::detail::OverflowMode::Checked>(value, value2, value)) {
            throw rdf4cpp::InvalidNode{std::format("{} parsing error: overflow", datatype)};
        }
        ++i;
    }

    if (neg) {
        value = -value;
    }
    return value;
}
#endif

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
    static constexpr size_t buf_sz = 5 + std::numeric_limits<F>::max_digits10 + std::max(2ul, detail::log10ceil(std::numeric_limits<F>::max_exponent10));
    std::array<char, buf_sz> buf;

    std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), value, std::chars_format::scientific);
    assert(res.ec == std::errc{});

    auto *e_ptr = std::find(buf.data(), res.ptr, 'e');
    assert(e_ptr != res.ptr); // serializing in scientific notation, there must be an 'e'
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
        static constexpr size_t buf_sz = 2 + std::numeric_limits<F>::max_exponent10 + std::numeric_limits<F>::max_digits10;
        std::array<char, buf_sz> buf;

        auto const res = std::to_chars(buf.data(), buf.data() + buf.size(), value, std::chars_format::fixed);
        assert(res.ec == std::errc{});

        std::string_view const s{buf.data(), static_cast<std::string::size_type>(res.ptr - buf.data())};
        return writer::write_str(s, writer);
    }

    return to_chars_canonical(value, writer);
}

} // namespace rdf4cpp::datatypes::registry::util

#endif  //RDF4CPP_REGISTRY_CHARCONVEXT_HPP
