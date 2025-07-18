/**
* @file
* Types for identifying entries in the registry and their helpers
*/

#ifndef RDF4CPP_REGISTRY_DATATYPEIRI_HPP
#define RDF4CPP_REGISTRY_DATATYPEIRI_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include <rdf4cpp/storage/identifier/LiteralType.hpp>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::datatypes::registry {

/**
 * A helper struct to visit all variants of either DatatypeID or DatatypeIDView
 *
 * @example Usage
 * @code
 * DatatypeID id{...};
 *
 * visit(DatatypeIDVisitor {
 *      [](LiteralType const fixed) {
 *          std::cout << "datatype id is fixed\n";
 *      },
 *      [](std::string const &dynamic) {
 *          std::cout << "datatype id is dynamic\n";
 *      }
 * }, id);
 * @endcode
 *
 * @tparam F The type of the function used to visit the fixed variant
 * @tparam D The type of the function used to visit the dynamic variant
 */
template<typename F, typename D>
struct DatatypeIDVisitor {
    F map_fixed;
    D map_dynamic;

    constexpr auto operator()(storage::identifier::LiteralType const fixed) noexcept(std::is_nothrow_invocable_v<F, storage::identifier::LiteralType const>) -> std::invoke_result_t<F, storage::identifier::LiteralType> {
        return std::invoke(this->map_fixed, fixed);
    }

    template<typename S>
    constexpr auto operator()(S &&other) noexcept(std::is_nothrow_invocable_v<S, decltype(std::forward<S>(other))>) -> std::invoke_result_t<D, decltype(std::forward<S>(other))> {
        return std::invoke(this->map_dynamic, std::forward<S>(other));
    }
};

template<typename F, typename D>
DatatypeIDVisitor(F, D) -> DatatypeIDVisitor<F, D>;

/**
 * A borrowed datatype id. Behaves to DatatypeID like std::string_view is to std::string.
 *
 * It either
 *   - contains a LiteralType, as the fixed variant, which means the type is known and lives in a fixed place in the registry
 *   - contains a std::string_view to the IRI of a non fixed datatype, which lives in a dynamic place in the registry
 *      and needs to be searched first
 *
 * Caution: You need to use the correct variant for the type you are searching, otherwise the registry
 *   will not find it. So usually this type should not be constructed manually, but instead obtained from a helper function, e.g.:
 *   - Literal::datatype_id
 *   - IRI::from_datatype_id
 */
struct DatatypeIDView {
private:
    using variant_t = std::variant<storage::identifier::LiteralType, std::string_view>;
    variant_t inner;
public:
    constexpr DatatypeIDView(storage::identifier::LiteralType fixed) noexcept
        : inner{fixed} {
        RDF4CPP_ASSERT(fixed.is_fixed());
    }

    explicit constexpr DatatypeIDView(std::string_view const other) noexcept
        : inner{other} {
    }

    [[nodiscard]] constexpr bool is_fixed() const noexcept {
        return this->inner.index() == 0;
    }

    [[nodiscard]] constexpr bool is_dynamic() const noexcept {
        return this->inner.index() == 1;
    }

    [[nodiscard]] constexpr storage::identifier::LiteralType get_fixed() const {
        return std::get<0>(this->inner);
    }

    [[nodiscard]] constexpr std::string_view get_dynamic() const {
        return std::get<1>(this->inner);
    }

    constexpr std::strong_ordering operator<=>(DatatypeIDView const &other) const noexcept = default;

    /**
     * Behaves like std::visit and is intended to be used with DatatypeIDVisitor
     */
    template<typename F>
    friend constexpr decltype(auto) visit(F &&f, DatatypeIDView const &self) noexcept(noexcept(std::visit(std::forward<F>(f), std::declval<variant_t const &>()))) {
        return std::visit(std::forward<F>(f), self.inner);
    }
};

/**
 * An owned datatype id. Behaves to DatatypeIDView as std::string to std::string_view.
 * The only difference is that it contains std::string instead of std::string_view for it's dynamic variant.
 * For more details see DatatypeIDView
 */
struct DatatypeID {
private:
    using variant_t = std::variant<storage::identifier::LiteralType, std::string>;
    variant_t inner;
public:
    explicit inline DatatypeID(DatatypeIDView const iri)
        : inner{visit(
                  DatatypeIDVisitor{
                          [](storage::identifier::LiteralType fixed) { return variant_t{fixed}; },
                          [](std::string_view other) { return variant_t{std::string{other}}; }},
                  iri)} {
    }

    explicit inline DatatypeID(storage::identifier::LiteralType const fixed) noexcept
        : inner{fixed} {
    }

    explicit inline DatatypeID(std::string const &other)
        : inner{other} {
    }

    explicit inline DatatypeID(std::string &&other) noexcept
        : inner{std::move(other)} {
    }

    inline operator DatatypeIDView() const noexcept {
        return visit(DatatypeIDVisitor{
                             [](storage::identifier::LiteralType fixed) { return DatatypeIDView{fixed}; },
                             [](std::string const &other) { return DatatypeIDView{other}; }},
                     this->inner);
    }

    [[nodiscard]] inline bool is_fixed() const {
        return this->inner.index() == 0;
    }

    [[nodiscard]] inline bool is_dynamic() const noexcept {
        return this->inner.index() == 1;
    }

    [[nodiscard]] inline storage::identifier::LiteralType get_fixed() const {
        return std::get<0>(this->inner);
    }

    [[nodiscard]] inline std::string const &get_dynamic() const {
        return std::get<1>(this->inner);
    }

    std::strong_ordering operator<=>(DatatypeID const &other) const noexcept = default;

    template<typename F>
    friend decltype(auto) visit(F && f, DatatypeID const &self) noexcept(noexcept(std::visit(std::forward<F>(f), std::declval<variant_t const &>()))) {
        return std::visit(std::forward<F>(f), self);
    }
};

} // rdf4cpp::datatypes::registry

#endif  //RDF4CPP_REGISTRY_DATATYPEIRI_HPP
