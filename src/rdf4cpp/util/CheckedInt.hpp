#ifndef RDF4CPP_CHECKEDINT_HPP
#define RDF4CPP_CHECKEDINT_HPP

#include <rdf4cpp/util/Int128.hpp>

namespace rdf4cpp::util {
/**
 * Wraps an integer type and keeps track of Overflows and similar Undefined Behavior.
 * Designed to be used in std::chrono::duration, but may be used standalone.
 * @tparam I Integer type to wrap
 */
template<typename I>
struct CheckedIntegral {
private:
    I value;
    bool invalid = false;

public:
    /**
     * Creates a CheckedIntegral from the underlying type and optionally a invalid specifier.
     * @note
     * Not explicit to allow arithmetic operations on CheckedIntegral and its underlying type.
     * @param value
     * @param invalid
     */
    constexpr CheckedIntegral(I value, bool invalid = false) noexcept : value(value), invalid(invalid) {}

    /**
     * Checks if the contained value is invalid.
     * @return
     */
    [[nodiscard]]constexpr  bool is_invalid() const noexcept {
        return invalid;
    }
    /**
     * Returns the contained value.
     * @note returned value is garbage, if is_invalid() evaluates to true.
     * @return
     */
    [[nodiscard]]constexpr  I get_value() const noexcept {
        return value;
    }

    constexpr std::partial_ordering operator<=>(const CheckedIntegral &other) const noexcept {
        if (this->invalid && other.invalid)
            return std::partial_ordering::equivalent;
        if (this->invalid != other.invalid)
            return std::partial_ordering::unordered;
        return this->value <=> other.value;
    }
    constexpr bool operator==(const CheckedIntegral &other) const noexcept {
        return (*this <=> other) == std::partial_ordering::equivalent;
    }

    constexpr CheckedIntegral &operator+=(const CheckedIntegral &other) noexcept {
        this->invalid |= other.invalid;
        this->invalid |= detail::add_checked<detail::OverflowMode::Checked>(this->value, other.value, this->value);
        return *this;
    }
    constexpr CheckedIntegral operator+(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r += other;
        return r;
    }
    constexpr CheckedIntegral &operator-=(const CheckedIntegral &other) noexcept {
        this->invalid |= other.invalid;
        this->invalid |= detail::sub_checked<detail::OverflowMode::Checked>(this->value, other.value, this->value);
        return *this;
    }
    constexpr CheckedIntegral operator-(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r -= other;
        return r;
    }
    constexpr CheckedIntegral operator-() const noexcept {
        return CheckedIntegral<I>{I{0}} - *this;
    }
    constexpr CheckedIntegral &operator*=(const CheckedIntegral &other) noexcept {
        this->invalid |= other.invalid;
        this->invalid |= detail::mul_checked<detail::OverflowMode::Checked>(this->value, other.value, this->value);
        return *this;
    }
    constexpr CheckedIntegral operator*(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r *= other;
        return r;
    }
    constexpr CheckedIntegral &operator/=(const CheckedIntegral &other) noexcept {
        if (this->invalid || other.invalid || other.value == 0) {
            this->invalid = true;
            return *this;
        }
        this->value = this->value / other.value;
        return *this;
    }
    constexpr CheckedIntegral operator/(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r /= other;
        return r;
    }
    constexpr CheckedIntegral &operator%=(const CheckedIntegral &other) noexcept {
        if (this->invalid || other.invalid || other.value == 0) {
            this->invalid = true;
            return *this;
        }
        this->value = this->value % other.value;
        return *this;
    }
    constexpr CheckedIntegral operator%(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r %= other;
        return r;
    }

    friend constexpr CheckedIntegral abs(CheckedIntegral const &val) noexcept {
        if constexpr (std::is_unsigned_v<I>) {
            return val;
        } else {
            if (val.value >= 0) {
                return val;
            }

            CheckedIntegral ret{0, val.invalid};
            ret.invalid |= detail::sub_checked<detail::OverflowMode::Checked>(I{0}, val.value, ret.value);
            return ret;
        }
    }

    template<typename To>
    constexpr CheckedIntegral<To> checked_cast() const noexcept {
        To r = 0;
        bool inv = detail::cast_checked<detail::OverflowMode::Checked>(value, r);
        return {r, inv || invalid};
    }

    template<typename From>
    requires std::convertible_to<From, I>
    constexpr CheckedIntegral(CheckedIntegral<From> f) : CheckedIntegral(f.template checked_cast<I>()) {} // chrono expects integer types to be implicitly convertible
    template<typename From>
    requires (!std::convertible_to<From, I>)
    constexpr explicit CheckedIntegral(CheckedIntegral<From> f) : CheckedIntegral(f.template checked_cast<I>()) {}
};
static_assert(std::convertible_to<rdf4cpp::util::CheckedIntegral<__int128>, rdf4cpp::util::CheckedIntegral<int64_t>>);

}  // namespace rdf4cpp::util

template<typename A, typename B>
struct std::common_type<rdf4cpp::util::CheckedIntegral<A>, rdf4cpp::util::CheckedIntegral<B>> {
    using type = rdf4cpp::util::CheckedIntegral<typename std::common_type<A, B>::type>;
};
static_assert(std::same_as<std::common_type<rdf4cpp::util::CheckedIntegral<__int128>, rdf4cpp::util::CheckedIntegral<int64_t>>::type,
                           rdf4cpp::util::CheckedIntegral<__int128>>);
static_assert(std::same_as<std::common_type<rdf4cpp::util::CheckedIntegral<__int128>, rdf4cpp::util::CheckedIntegral<int64_t>, std::intmax_t>::type,
                           rdf4cpp::util::CheckedIntegral<__int128>>);

#endif  //RDF4CPP_CHECKEDINT_HPP
