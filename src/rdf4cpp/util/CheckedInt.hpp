#ifndef RDF4CPP_CHECKEDINT_HPP
#define RDF4CPP_CHECKEDINT_HPP

#include <concepts>

namespace rdf4cpp::util {
/**
 * Wraps an integer type and keeps track of Overflows and similar Undefined Behavior.
 * Designed to be used in std::chrono::duration, but may be used standalone.
 * @tparam I Integer type to wrap
 */
template<std::integral I>
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
    [[nodiscard]] constexpr bool is_invalid() const noexcept {
        return invalid;
    }

    /**
     * Returns the contained value.
     * @note returned value is garbage, if is_invalid() evaluates to true.
     * @return
     */
    [[nodiscard]] constexpr I get_value() const noexcept {
        return value;
    }

    constexpr std::partial_ordering operator<=>(const CheckedIntegral &other) const noexcept {
        if (this->invalid && other.invalid)
            return std::partial_ordering::equivalent;
        if (this->invalid != other.invalid)
            return std::partial_ordering::unordered;
        return this->value <=> other.value;
    }

    constexpr CheckedIntegral &operator+=(const CheckedIntegral &other) noexcept {
        this->invalid |= other.invalid;
        this->invalid |= __builtin_add_overflow(this->value, other.value, &this->value);
        return *this;
    }
    constexpr CheckedIntegral operator+(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r += other;
        return r;
    }
    constexpr CheckedIntegral &operator-=(const CheckedIntegral &other) noexcept {
        this->invalid |= other.invalid;
        this->invalid |= __builtin_sub_overflow(this->value, other.value, &this->value);
        return *this;
    }
    constexpr CheckedIntegral operator-(const CheckedIntegral &other) const noexcept {
        CheckedIntegral r = *this;
        r -= other;
        return r;
    }
    constexpr CheckedIntegral &operator*=(const CheckedIntegral &other) noexcept {
        this->invalid |= other.invalid;
        this->invalid |= __builtin_mul_overflow(this->value, other.value, &this->value);
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

    friend constexpr CheckedIntegral abs(CheckedIntegral const &val) noexcept {
        if constexpr (std::is_unsigned_v<I>) {
            return val;
        } else {
            if (val.value >= 0) {
                return val;
            }

            CheckedIntegral ret{0, val.invalid};
            ret.invalid |= __builtin_sub_overflow(0, val.value, &ret.value);
            return ret;
        }
    }
};

}  // namespace rdf4cpp::util

#endif  //RDF4CPP_CHECKEDINT_HPP
