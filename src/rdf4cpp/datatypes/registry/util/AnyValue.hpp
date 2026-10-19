#ifndef RDF4CPP_REGISTRY_UTIL_ANYVALUE_HPP
#define RDF4CPP_REGISTRY_UTIL_ANYVALUE_HPP

#include <any>
#include <concepts>
#include <type_traits>
#include <utility>

namespace rdf4cpp::datatypes::registry::util {

/**
 * Wrapper to fix std::expected<std::any, E> on libstdc++ which is still an issue on Clang 23.
 *
 * std::expected<std::any, E> cannot be used directly, because std::any is constructible from
 * (almost) any type, including std::expected<std::any, E> itself. This makes the constraints of std::expected's
 * converting constructors depend on themselves, which clang rejects.
 * This wrapper is only constructible from std::any, which breaks the cycle.
 *
 * AnyValue is intentionally move-only, so that it cannot accidentally be wrapped into another std::any
 * (std::any requires copy-constructible types). Access the contained value via AnyValue::value.
 */
struct AnyValue {
    std::any value;

    template<typename T> requires std::same_as<std::remove_cvref_t<T>, std::any>
    AnyValue(T &&v) noexcept(std::is_nothrow_constructible_v<std::any, T>) : value{std::forward<T>(v)} {
    }

    AnyValue(AnyValue const &) = delete;
    AnyValue(AnyValue &&) noexcept = default;
    AnyValue &operator=(AnyValue const &) = delete;
    AnyValue &operator=(AnyValue &&) noexcept = default;
};

}  // namespace rdf4cpp::datatypes::registry::util

#endif  // RDF4CPP_REGISTRY_UTIL_ANYVALUE_HPP
