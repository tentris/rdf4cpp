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
    std::array<char, N> value;

    constexpr ConstexprString(const char (&str)[N]) noexcept {
        std::copy_n(str, N, value.data());
    }
    constexpr ConstexprString(std::string_view v) {
        if (v.size()+1 != N)
            throw std::invalid_argument{"size missmatch"};
        std::copy_n(v.data(), N-1, value.data());
        value[N-1] = 0;
    }
    constexpr ConstexprString() = default;

    constexpr operator std::string_view() const noexcept {
        return std::string_view{value.data(), value.size() - 1};
    }

    [[nodiscard]] constexpr char const *c_str() const noexcept {
        return value.data();
    }

    template<size_t M>
    constexpr std::strong_ordering operator<=>(ConstexprString<M> const &other) const noexcept {
        auto min = std::min(M, N);
        for (size_t i = 0; i < min; ++i) {
            std::strong_ordering cmp = this->value[i] <=> other.value[i];
            if (cmp != std::strong_ordering::equal)
                return cmp;
        }
        std::strong_ordering cmp = N <=> M;
        return cmp;
    }

    template<size_t M>
    constexpr bool operator==(ConstexprString<M> const &other) const noexcept {
        if (M == N) {
            return (*this <=> other == std::strong_ordering::equal);
        } else {
            return false;
        }
    }

    [[nodiscard]] constexpr size_t size() const noexcept {
        return N;
    }

    template<size_t M>
    constexpr ConstexprString<M+N-1> operator+(const ConstexprString<M>& other) const noexcept {
        ConstexprString<M+N-1> r{};
        std::copy_n(this->value.data(), N-1, r.value.data());
        std::copy_n(other.value.data(), M-1, r.value.data() + (N-1));
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
        return formatter<string_view>::format(static_cast<std::string_view>(s), ctx);
    }
};

#endif  //RDF4CPP_REGISTRY_UTIL_CONSTEXPRSTRING_HPP
