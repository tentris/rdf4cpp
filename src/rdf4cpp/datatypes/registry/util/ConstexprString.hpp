#ifndef RDF4CPP_REGISTRY_UTIL_CONSTEXPRSTRING_HPP
#define RDF4CPP_REGISTRY_UTIL_CONSTEXPRSTRING_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>

namespace rdf4cpp::datatypes::registry::util {
/**
 * Literal class type that wraps a constant expression string.
 *
 * Uses implicit conversion to allow templates to *seemingly* accept constant strings.
 *
 * Extended version of https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/.
 */
template<size_t N>
struct ConstexprString {
    std::array<char, N> value{};

    constexpr ConstexprString() noexcept = default;

    /**
     * @param str null-terminated char array/string literal
     */
    constexpr ConstexprString(char const (&str)[N]) noexcept {
        std::copy_n(str, N, value.data());
    }

    constexpr ConstexprString(std::string_view v) {
        if (v.size() != N - 1) {
            throw std::invalid_argument{"size mismatch"};
        }

        std::copy_n(v.data(), N - 1, value.data());
        // implicitly null terminated by default init of array
    }

    [[nodiscard]] constexpr char const *data() const noexcept {
        return value.data();
    }

    [[nodiscard]] constexpr char *data() noexcept {
        return value.data();
    }

    [[nodiscard]] static constexpr size_t size() noexcept {
        return N - 1; // exclude null terminator
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return std::string_view{data(), size()};
    }

    constexpr operator std::string_view() const noexcept {
        return view();
    }

    template<size_t M>
    constexpr std::strong_ordering operator<=>(ConstexprString<M> const &other) const noexcept {
        return view() <=> other.view();
    }

    template<size_t M>
    constexpr bool operator==(ConstexprString<M> const &other) const noexcept {
        if constexpr (M == N) {
            return value == other.value;
        } else {
            return false;
        }
    }

    template<size_t M>
    constexpr ConstexprString<M+N-1> operator+(ConstexprString<M> const &other) const noexcept {
        ConstexprString<M+N-1> r{}; // only include a single null terminator N+M would include 2
        std::copy_n(data(), size(), r.data());
        std::copy_n(other.data(), other.size(), r.data() + size());
        // implicitly null terminated by default init of array
        return r;
    }
};

template<ConstexprString Data>
struct ConstexprStringHolder {
    static constexpr auto value = Data;
};

}  // namespace rdf4cpp::datatypes::registry::util

template<size_t N>
struct std::formatter<rdf4cpp::datatypes::registry::util::ConstexprString<N>> : std::formatter<std::string_view> {
    auto format(rdf4cpp::datatypes::registry::util::ConstexprString<N> const &s, format_context& ctx) const {
        return formatter<string_view>::format(s.view(), ctx);
    }
};

#endif  //RDF4CPP_REGISTRY_UTIL_CONSTEXPRSTRING_HPP
