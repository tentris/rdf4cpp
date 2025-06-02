#ifndef RDF4CPP_BIGDECIMAL_H
#define RDF4CPP_BIGDECIMAL_H

#include <functional>
#include <sstream>
#include <string>
#include <string_view>

#include <boost/multiprecision/cpp_int.hpp>
#include <dice/hash.hpp>

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/Int128.hpp>

namespace rdf4cpp {
enum struct RoundingMode {
    Floor,
    Ceil,
    Round,
    Trunc,
};

enum struct DecimalError {
    Overflow,
    NotDefined,  // aka NotANumber
};

template<typename T>
concept BigDecimalBaseType = std::numeric_limits<T>::is_specialized && !std::floating_point<T>;

template<BigDecimalBaseType UnscaledValue_t = util::Int128, BigDecimalBaseType Exponent_t = uint32_t>
requires(!std::signed_integral<Exponent_t> && !std::unsigned_integral<UnscaledValue_t>)
struct BigDecimal {
    // the entire class is loosely based on OpenJDKs BigDecimal: https://github.com/AdoptOpenJDK/openjdk-jdk11/blob/master/src/java.base/share/classes/java/math/BigDecimal.java

private:
    UnscaledValue_t unscaled_value;
    Exponent_t exponent;

    using OverflowMode = util::detail::OverflowMode;

    static constexpr uint32_t base = 10;

    template<OverflowMode m, typename To, typename From>
    static constexpr bool cast_checked(const From &f, To &result) noexcept {
        if constexpr (std::is_integral_v<To> && std::is_integral_v<From> && m == OverflowMode::Checked) {
            if (!std::in_range<To>(f))
                return true;
        } else if (m == OverflowMode::Checked) {
            try {
                result = static_cast<To>(f);
            } catch (std::overflow_error const &) {
                return true;
            } catch (std::range_error const &) {
                return true;
            }
            return false;
        }
        result = static_cast<To>(f);
        return false;
    }

    static constexpr UnscaledValue_t abs(const UnscaledValue_t &value) noexcept {
        if constexpr (std::is_integral_v<UnscaledValue_t> && !std::is_signed_v<UnscaledValue_t>) {
            return value;
        } else {
            return value < 0 ? -value : value;
        }
    }

public:
    constexpr BigDecimal() noexcept : BigDecimal{0, 0} {
    }

    /**
     * creates a BigDecimal from its components.
     * it has the value of unscaled_value * pow(10, -exponent).
     * @param unscaled_value
     * @param exponent
     */
    constexpr BigDecimal(const UnscaledValue_t &unscaled_value, Exponent_t exponent) noexcept
        : unscaled_value(unscaled_value), exponent(exponent) {}

    /**
     * parses a BigDecimal from a string_view.
     * may include a leading sign and one decimal point ., everything else needs to be numeric.
     * @param value
     * @throw rdf4cpp::InvalidNode if a invalid char is found, or on exceeding the types numeric limits
     */
    constexpr explicit BigDecimal(std::string_view value) : unscaled_value(0), exponent(0) {
        bool begin = true;
        bool decimal = false;
        bool neg = false;
        for (const char c : value) {
            if (begin) {
                begin = false;
                if (c == '-') {
                    neg = true;
                    continue;
                } else if (c == '+') {
                    continue;
                }
            }
            if (c == '.') {
                if (decimal) {
                    throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: more than one . found"};
                }
                decimal = true;
                continue;
            }
            if (c < '0' || c > '9') {
                throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: non-numeric char found"};
            }
            auto n = c - '0';
            if (util::detail::mul_checked<OverflowMode::Checked>(unscaled_value, UnscaledValue_t{base}, unscaled_value)) {
                throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: unscaled_value overflow"};
            }
            if (util::detail::add_checked<OverflowMode::Checked>(unscaled_value, UnscaledValue_t{n}, unscaled_value)) {
                throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: unscaled_value overflow"};
            }
            if (decimal) {
                if (util::detail::add_checked<OverflowMode::Checked>(exponent, Exponent_t{1}, exponent)) {
                    throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: exponent overflow"};
                }
            }
        }
        if (unscaled_value == 0) {
            neg = false;
        }
        if (neg) {
            unscaled_value = -unscaled_value;
        }
    }

    /**
     * converts a uint32_t to a BigDecimal
     * @param value
     */
    constexpr explicit BigDecimal(uint32_t value) noexcept : BigDecimal(UnscaledValue_t{value}, 0) {}

    /**
     * converts a uint64_t to a BigDecimal
     * @param value
     */
    constexpr explicit BigDecimal(uint64_t value) noexcept : BigDecimal(UnscaledValue_t{value}, 0) {}

    /**
     * converts a int32_t to a BigDecimal
     * @param value
     */
    constexpr explicit BigDecimal(int32_t value) noexcept : BigDecimal(static_cast<UnscaledValue_t>(value), 0) {}

    /**
     * converts a int64_t to a BigDecimal
     * @param value
     */
    constexpr explicit BigDecimal(int64_t value) noexcept : BigDecimal(static_cast<UnscaledValue_t>(value), 0) {}

    /**
     * converts a UnscaledValue_t to a BigDecimal
     * @param value
     */
    constexpr explicit BigDecimal(const UnscaledValue_t &value) noexcept
        requires(!std::is_same_v<UnscaledValue_t, int32_t> && !std::is_same_v<UnscaledValue_t, int64_t>)
        : BigDecimal(value, 0) {}

    /**
     * converts a float to a BigDecimal.
     * this conversion might not be exact, due to the built in limitations of floats.
     * if you have the possibility, use one of the other constructors.
     * @param value
     * @throw std::overflow_error on exceeding the types numeric limits
     */
    constexpr explicit BigDecimal(float value) : BigDecimal(static_cast<double>(value)) {}

private:
    void from_double_direct(double value)
    requires(std::numeric_limits<UnscaledValue_t>::digits >= std::numeric_limits<boost::multiprecision::int1024_t>::digits)
    {
        // most of the algorithm is from OpenJDK: https://github.com/AdoptOpenJDK/openjdk-jdk11/blob/19fb8f93c59dfd791f62d41f332db9e306bc1422/src/java.base/share/classes/java/math/BigDecimal.java#L915
        if (std::isinf(value) || std::isnan(value))
            throw std::invalid_argument{"value is NaN or infinity"};
        // this might fail on anything that is not x86-32/64
        static_assert(std::endian::native == std::endian::little, "BigDecimal{double} is only tested on x86-32/64 and might not work on other systems");
        // double is an IEEE 754 64-bit floating point value
        // memory layout:
        // sign | exponent  | fraction
        // 63   | 62 ... 52 | 51 ... 0
        // see https://en.wikipedia.org/wiki/Double-precision_floating-point_format for more info (and a better graphic)
        auto v = std::bit_cast<uint64_t>(value);
        bool const neg = (v >> 63) != 0;
        auto ex = static_cast<int>((v >> 52) & 0x7ffL);
        uint64_t significand = ex == 0
                                       ? (v & ((1L << 52) - 1)) << 1
                                       : (v & ((1L << 52) - 1)) | (1L << 52);
        ex -= 1075;
        if (significand == 0) {
            return;
        }
        while (significand & 1) {
            significand >>= 1;
            ++ex;
        }
        static constexpr const char *exc = "double to BigDecimal overflow";
        if (ex == 0) {
            unscaled_value = UnscaledValue_t{significand};
        } else if (ex < 0) {
            exponent = static_cast<Exponent_t>(-ex);
            UnscaledValue_t e{0};
            if (util::detail::pow_checked<OverflowMode::Checked>(UnscaledValue_t{5}, -ex, e))
                throw std::overflow_error{exc};
            if (util::detail::mul_checked<OverflowMode::Checked>(UnscaledValue_t{significand}, e, unscaled_value))
                throw std::overflow_error{exc};
        } else {
            UnscaledValue_t e{0};
            if (util::detail::pow_checked<OverflowMode::Checked>(UnscaledValue_t{2}, ex, e))
                throw std::overflow_error{exc};
            if (util::detail::mul_checked<OverflowMode::Checked>(UnscaledValue_t{significand}, e, unscaled_value))
                throw std::overflow_error{exc};
        }
        if (neg)
            unscaled_value = -unscaled_value;
        normalize();
    }

public:
    /**
     * converts a double to a BigDecimal.
     * this conversion might not be exact, due to the built in limitations of doubles.
     * if you have the possibility, use one of the other constructors.
     * @param value
     * @throw std::overflow_error on exceeding the types numeric limits
     */
    explicit BigDecimal(double value) : unscaled_value(0), exponent(0) {
        if constexpr (std::numeric_limits<UnscaledValue_t>::digits >= std::numeric_limits<boost::multiprecision::int1024_t>::digits) {
            from_double_direct(value);
        } else {
            BigDecimal<boost::multiprecision::checked_int1024_t> const v{value};
            if (cast_checked<OverflowMode::Checked>(v.get_unscaled_value(), unscaled_value))
                throw std::overflow_error{"double to BigDecimal overflow"};
            if (cast_checked<OverflowMode::Checked>(v.get_exponent(), exponent))
                throw std::overflow_error{"double to BigDecimal overflow"};
        }
    }

    /**
     * converts this BigDecimal to its smallest internal representation.
     */
    constexpr void normalize() noexcept {
        normalize(unscaled_value, exponent);
    }

    /**
     * converts this BigDecimal to its smallest internal representation.
     */
    static constexpr void normalize(UnscaledValue_t &unscaled_value, Exponent_t &exponent) noexcept {
        while (exponent > 0 && unscaled_value % base == 0) {
            unscaled_value /= base;
            --exponent;
        }
    }

    [[nodiscard]] constexpr Exponent_t get_exponent() const noexcept {
        return exponent;
    }

    [[nodiscard]] constexpr UnscaledValue_t get_unscaled_value() const noexcept {
        return unscaled_value;
    }

    [[nodiscard]] constexpr bool positive() const noexcept {
        return unscaled_value >= 0;
    }

private:
    // op_checked has to be add_checked or sub_checked with the same OverflowMode
    template<OverflowMode m, bool (*op_checked)(const UnscaledValue_t &t, const UnscaledValue_t &o, UnscaledValue_t &result)>
    constexpr bool add_or_sub(const BigDecimal &other, BigDecimal &result) const noexcept {
        UnscaledValue_t t = this->unscaled_value;
        UnscaledValue_t o = other.unscaled_value;
        Exponent_t new_exp = std::max(this->exponent, other.exponent);
        if (this->exponent < new_exp) {
            UnscaledValue_t ex{0};
            if (util::detail::pow_checked<m>(UnscaledValue_t{base}, new_exp - this->exponent, ex))
                return true;
            if (util::detail::mul_checked<m>(t, ex, t))
                return true;
        } else if (other.exponent < new_exp) {
            UnscaledValue_t ex{0};
            if (util::detail::pow_checked<m>(UnscaledValue_t{base}, new_exp - other.exponent, ex))
                return true;
            if (util::detail::mul_checked<m>(o, ex, o))
                return true;
        }
        UnscaledValue_t res = 0;
        if (op_checked(t, o, res))
            return true;
        result = BigDecimal{res, new_exp};
        return false;
    }

    template<OverflowMode m>
    constexpr bool mul(const BigDecimal &other, BigDecimal &result) const noexcept {
        UnscaledValue_t v{0};
        if (util::detail::mul_checked<m>(this->unscaled_value, other.unscaled_value, v))
            return true;
        Exponent_t e{0};
        if (util::detail::add_checked<m>(this->exponent, other.exponent, e))
            return true;
        result = BigDecimal{v, e};
        return false;
    }

    constexpr static BigDecimal handle_rounding(UnscaledValue_t v, Exponent_t e, UnscaledValue_t rem, RoundingMode m) noexcept {
        switch (m) {
            case RoundingMode::Trunc:
                return BigDecimal{v, e};
            case RoundingMode::Floor:
                if (v >= 0 || rem == 0)
                    return BigDecimal{v, e};
                else
                    return BigDecimal{v - 1, e};
            case RoundingMode::Ceil:
                if (v < 0 || rem == 0)
                    return BigDecimal{v, e};
                else
                    return BigDecimal{v + 1, e};
            case RoundingMode::Round:
                if (rem < 0)
                    rem = -rem;
                if (v < 0) {
                    if (rem >= 5)
                        return BigDecimal{v - 1, e};
                    else
                        return BigDecimal{v, e};
                } else {
                    if (rem >= 5)
                        return BigDecimal{v + 1, e};
                    else
                        return BigDecimal{v, e};
                }
            default:
                assert(false);
                __builtin_unreachable();
        }
    }

    template<OverflowMode m>
    constexpr bool div(const BigDecimal &other, Exponent_t max_scale_increase, RoundingMode mode, BigDecimal &result) const noexcept {
        if (this->unscaled_value == 0) {
            result = BigDecimal{0, 0};
            return false;
        }
        UnscaledValue_t t = this->unscaled_value;
        Exponent_t ex = this->exponent;
        UnscaledValue_t div = other.unscaled_value;
        if (ex >= other.exponent) {
            if (util::detail::sub_checked<m>(ex, other.exponent, ex))
                return true;
        } else {
            Exponent_t tmp{0};
            if (util::detail::sub_checked<m>(other.exponent, ex, tmp))
                return true;
            if (util::detail::pow_checked<m>(Exponent_t{base}, tmp, tmp))
                return true;
            UnscaledValue_t tmp2{0};
            if (cast_checked<m>(tmp, tmp2))
                return true;
            if (util::detail::mul_checked<m>(t, tmp2, t))
                return true;
            ex = 0;
        }
        UnscaledValue_t res = t / div;
        UnscaledValue_t rem = t % div;
        while (rem != 0) {
            if (max_scale_increase == 0) {
                if (util::detail::mul_checked<m>(rem, UnscaledValue_t{base}, rem))
                    return true;
                result = handle_rounding(res, ex, rem / div, mode);
                return false;
            }
            if constexpr (std::is_integral_v<Exponent_t>) {
                if (ex == std::numeric_limits<Exponent_t>::max()) {
                    if (util::detail::mul_checked<m>(rem, UnscaledValue_t{base}, rem))
                        return true;
                    result = handle_rounding(res, ex, rem / div, mode);
                    return false;
                }
            }
            ++ex;
            if (util::detail::mul_checked<m>(res, UnscaledValue_t{base}, res))
                return true;
            if (util::detail::mul_checked<m>(rem, UnscaledValue_t{base}, rem))
                return true;
            res += rem / div;
            rem = rem % div;
            --max_scale_increase;
        }
        result = BigDecimal{res, ex};
        return false;
    }

    template<OverflowMode m>
    constexpr bool pow(unsigned int n, BigDecimal &result) const noexcept {
        BigDecimal r{1};

        for (unsigned int i = 0; i < n; ++i) {
            if (r.mul<m>(*this, r))
                return true;
        }
        result = r;
        return false;
    }

    static double cast_unscaled_to_double_safe(UnscaledValue_t const &value) noexcept {
        // There is a bug in boost versions <1.86.0
        // that can cause a crash in the cast `static_cast<double>(cpp_int)`
        // https://github.com/boostorg/multiprecision/issues/553

#if BOOST_VERSION < 108600
        // workaround from https://github.com/scylladb/scylladb/commit/51d09e6a2a407ea9b9ad380ba30e5558a25bb8be#diff-81e6758bbedbe566a51706c5af1ea68be225c36feb68983c3de0a69138f01264R73
        static auto const min = UnscaledValue_t{std::numeric_limits<double>::lowest()};
        static auto const max = UnscaledValue_t{std::numeric_limits<double>::max()};
        if (value < min) {
            return -std::numeric_limits<double>::infinity();
        } else if (value > max) {
            return std::numeric_limits<double>::infinity();
        }
#endif // BOOST_VERSION

        return static_cast<double>(value);
    }

public:
    /**
     * unary minus of this BigDecimal.
     * may overflow.
     * @return
     */
    [[nodiscard]] constexpr BigDecimal operator-() const noexcept {
        return BigDecimal(-this->unscaled_value, this->exponent);
    }
    /**
     * unary minus of this BigDecimal.
     * checks overflow.
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> unary_minus_checked() const noexcept {
        if constexpr (std::is_integral_v<UnscaledValue_t>) {
            if (std::numeric_limits<UnscaledValue_t>::min() == unscaled_value)
                return nonstd::make_unexpected(DecimalError::Overflow);
        }
        return BigDecimal(-this->unscaled_value, this->exponent);
    }


    /**
     * unary plus (nop) of this BigDecimal.
     * @return
     */
    [[nodiscard]] constexpr BigDecimal operator+() const noexcept {
        return *this;
    }

    /**
     * addition of two BigDecimals.
     * may overflow.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr BigDecimal operator+(const BigDecimal &other) const noexcept {
        BigDecimal res{0};
        add_or_sub<OverflowMode::UndefinedBehavior, util::detail::add_checked<OverflowMode::UndefinedBehavior>>(other, res);
        return res;
    }
    /**
     * addition of two BigDecimals.
     * checks overflow.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> add_checked(const BigDecimal &other) const noexcept {
        BigDecimal res{0};
        if (add_or_sub<OverflowMode::Checked, util::detail::add_checked<OverflowMode::Checked>>(other, res))
            return nonstd::make_unexpected(DecimalError::Overflow);
        return res;
    }

    /**
     * addition of two BigDecimals.
     * may overflow.
     * @param other
     * @return
     */
    constexpr BigDecimal &operator+=(const BigDecimal &other) noexcept {
        *this = *this + other;
        return *this;
    }

    /**
     * subtraction of two BigDecimals.
     * may overflow.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr BigDecimal operator-(const BigDecimal &other) const noexcept {
        BigDecimal res{0};
        add_or_sub<OverflowMode::UndefinedBehavior, util::detail::sub_checked<OverflowMode::UndefinedBehavior>>(other, res);
        return res;
    }

    /**
     * subtraction of two BigDecimals.
     * checks overflow.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> sub_checked(const BigDecimal &other) const noexcept {
        BigDecimal res{0};
        if (add_or_sub<OverflowMode::Checked, util::detail::sub_checked<OverflowMode::Checked>>(other, res))
            return nonstd::make_unexpected(DecimalError::Overflow);
        return res;
    }

    /**
     * subtraction of two BigDecimals.
     * may overflow.
     * @param other
     * @return
     */
    constexpr BigDecimal operator-=(const BigDecimal &other) noexcept {
        *this = *this - other;
        return *this;
    }

    /**
     * multiplication of two BigDecimals.
     * may overflow.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr BigDecimal operator*(const BigDecimal &other) const noexcept {
        BigDecimal res{0};
        mul<OverflowMode::UndefinedBehavior>(other, res);
        return res;
    }

    /**
     * multiplication of two BigDecimals.
     * checks overflow.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> mul_checked(const BigDecimal &other) const noexcept {
        BigDecimal res{0};
        if (mul<OverflowMode::Checked>(other, res))
            return nonstd::make_unexpected(DecimalError::Overflow);
        return res;
    }

    /**
     * multiplication of two BigDecimals.
     * may overflow.
     * @param other
     * @return
     */
    constexpr BigDecimal &operator*=(const BigDecimal &other) noexcept {
        *this = *this * other;
        return *this;
    }

    /**
     * division of two BigDecimals.
     * may overflow.
     * after 1000 decimal places have been added, stops dividing and floor.
     * dividing by 0 is undefined behavior.
     * @param other
     * @return
     */
    [[nodiscard]] constexpr BigDecimal operator/(const BigDecimal &other) const noexcept {
        return div(other, 1000);
    }

    /**
     * division of two BigDecimals.
     * may overflow.
     * dividing by 0 is undefined behavior.
     * @param other
     * @param max_scale_increase after this many decimal places have been added, stop dividing and round
     * @param mode rounding mode to use
     * @return
     */
    [[nodiscard]] constexpr BigDecimal div(const BigDecimal &other, Exponent_t max_scale_increase, RoundingMode mode = RoundingMode::Floor) const noexcept {
        if (other.unscaled_value == 0)
            return BigDecimal{0, 0};  // undefined behavior (cpp_int throws)
        BigDecimal res{0};
        div<OverflowMode::UndefinedBehavior>(other, max_scale_increase, mode, res);
        return res;
    }

    /**
     * division of two BigDecimals.
     * checks overflow and division by 0.
     * @param other
     * @param max_scale_increase after this many decimal places have been added, stop dividing and round
     * @param mode rounding mode to use
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> div_checked(const BigDecimal &other, Exponent_t max_scale_increase, RoundingMode mode = RoundingMode::Floor) const noexcept {
        if (other.unscaled_value == 0)
            return nonstd::make_unexpected(DecimalError::NotDefined);
        BigDecimal res{0};
        if (div<OverflowMode::Checked>(other, max_scale_increase, mode, res))
            return nonstd::make_unexpected(DecimalError::Overflow);
        return res;
    }

    /**
     * division of two BigDecimals.
     * may overflow.
     * after 1000 decimal places have been added, stops dividing and floor.
     * dividing by 0 is undefined behavior.
     * @param other
     * @return
     */
    constexpr BigDecimal &operator/=(const BigDecimal &other) noexcept {
        *this = *this / other;
        return *this;
    }

    /**
     * raises a BigDecimal to the power of a int.
     * may overflow.
     * @param n
     * @return
     */
    [[nodiscard]] constexpr BigDecimal pow(unsigned int n) const noexcept {
        BigDecimal r{0};
        pow<OverflowMode::UndefinedBehavior>(n, r);
        return r;
    }

    /**
     * raises a BigDecimal to the power of a int.
     * checks overflow.
     * @param n
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> pow_checked(unsigned int n) const noexcept {
        BigDecimal r{0};
        if (pow<OverflowMode::Checked>(n, r))
            return nonstd::make_unexpected(DecimalError::Overflow);
        return r;
    }

    /**
     * rounds a BigDecimal with a specified RoundingMode.
     * @param mode
     * @return
     */
    [[nodiscard]] constexpr BigDecimal round(RoundingMode mode) const noexcept {
        if (exponent == 0)
            return *this;
        UnscaledValue_t v{base};
        if (util::detail::pow_checked<OverflowMode::Checked>(v, exponent, v))
            return BigDecimal{0, 0};  // base pow exponent overflows and this did not, we have to be close to 0
        UnscaledValue_t rem = unscaled_value % v;
        rem = rem * 10 / v;
        v = unscaled_value / v;
        return handle_rounding(v, 0, rem, mode);
    }

    /**
     * the absolute value of a BigDecimal.
     * may overflow.
     * @return
     */
    [[nodiscard]] constexpr BigDecimal abs() const noexcept {
        if (positive())
            return *this;
        else
            return -*this;
    }

    /**
     * the absolute value of a BigDecimal.
     * checks overflow.
     * @return
     */
    [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> abs_checked() const noexcept {
        if (positive())
            return *this;
        else
            return this->unary_minus_checked();
    }

    /**
     * comparison between BigDecimals.
     * @param other
     * @return
     */
    constexpr std::strong_ordering operator<=>(const BigDecimal &other) const noexcept {
        if (this->positive() != other.positive())
            return this->positive() ? std::strong_ordering::greater : std::strong_ordering::less;
        UnscaledValue_t t = this->unscaled_value;
        UnscaledValue_t o = other.unscaled_value;
        if (this->exponent > other.exponent) {
            UnscaledValue_t b{base};
            if (util::detail::pow_checked<OverflowMode::Checked>(b, this->exponent - other.exponent, b))
                return std::strong_ordering::less;  // t does fit into the same precision, while o does not
            if (util::detail::mul_checked<OverflowMode::Checked>(o, b, o))
                return std::strong_ordering::less;  // t does fit into the same precision, while o does not
        } else if (this->exponent < other.exponent) {
            UnscaledValue_t b{base};
            if (util::detail::pow_checked<OverflowMode::Checked>(b, other.exponent - this->exponent, b))
                return std::strong_ordering::greater;  // o does fit into the same precision, while t does not
            if (util::detail::mul_checked<OverflowMode::Checked>(t, b, t))
                return std::strong_ordering::greater;  // o does fit into the same precision, while t does not
        }
        if (t < o)
            return std::strong_ordering::less;
        else if (t > o)
            return std::strong_ordering::greater;
        else
            return std::strong_ordering::equivalent;
    };
    /**
     * equality between BigDecimals.
     * @param other
     * @return
     */
    constexpr bool operator==(const BigDecimal &other) const noexcept {
        return *this <=> other == std::strong_ordering::equivalent;
    }
    /**
     * equality between a BigDecimal and a int (mainly for constants)
     * @param t
     * @param other
     * @return
     */
    friend bool operator==(const BigDecimal &t, int other) noexcept {
        return t == BigDecimal{other, 0};
    }
    /**
     * equality between a BigDecimal and a int (mainly for constants)
     * @param t
     * @param other
     * @return
     */
    friend bool operator==(int t, const BigDecimal &other) noexcept {
        return other == t;
    }

    /**
     * conversion to a double
     * @return
     */
    [[nodiscard]] explicit operator double() const noexcept {
        double const v = cast_unscaled_to_double_safe(unscaled_value) * std::pow(static_cast<double>(base), -static_cast<double>(exponent));
        if (!std::isnan(v) && !std::isinf(v))
            return v;
        // even Javas BigDecimal has no better solution
        auto const s = static_cast<std::string>(*this);
        return std::stod(s);
    }

    /**
     * conversion to a float
     * @return
     */
    [[nodiscard]] explicit operator float() const noexcept {
        return static_cast<float>(static_cast<double>(*this));
    }

    /**
     * conversion to a UnscaledValue_t
     * @return
     */
    [[nodiscard]] constexpr explicit operator UnscaledValue_t() const noexcept {
        if (exponent == 0)
            return unscaled_value;
        return round(RoundingMode::Trunc).unscaled_value;
    }

    /**
     * conversion to a string
     * @return
     */
    [[nodiscard]] explicit operator std::string() const noexcept {
        if (unscaled_value == 0)
            return "0.0";
        std::stringstream s{};
        UnscaledValue_t v = unscaled_value;
        Exponent_t ex = exponent;
        bool hasDot = false;
        while (v != 0) {
            if (!hasDot && ex == 0) {
                if (s.view().empty()) {
                    s << '0';
                }
                s << '.';
                hasDot = true;
            } else {
                --ex;
            }
            using namespace std;
            auto c = static_cast<uint32_t>(abs(v % base));
            if (hasDot || c != 0 || !s.view().empty()) {  // skip trailing 0s
                s << c;
            }
            v /= base;
        }
        if (!hasDot) {
            for (Exponent_t i = 0; i < ex; ++i) {
                s << '0';
            }
            s << ".0";
        }
        if (!positive()) {
            s << '-';
        }
        std::string_view sv = s.view();
        return std::string{sv.rbegin(), sv.rend()};
    }

    /**
     * writing a BigDecimal into a stream.
     * @param str
     * @param bn
     * @return
     */
    friend std::ostream &operator<<(std::ostream &str, const BigDecimal &bn) {
        auto s = static_cast<std::string>(bn);
        str << s;
        return str;
    }

    /**
     * combined hash of a BigDecimals components hashes.
     * @return
     */
    template<dice::hash::Policies::HashPolicy Policy = dice::hash::Policies::wyhash>
    [[nodiscard]] size_t hash() const {
        BigDecimal n = *this;
        n.normalize();

        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(n.unscaled_value, n.exponent));
    }
};

template<typename UnscaledValue_t, typename Exponent_t>
std::string to_string(const BigDecimal<UnscaledValue_t, Exponent_t> &r) noexcept {
    return static_cast<std::string>(r);
}

template<typename UnscaledValue_t, typename Exponent_t>
BigDecimal<UnscaledValue_t, Exponent_t> pow(const BigDecimal<UnscaledValue_t, Exponent_t> &r, unsigned int n) noexcept {
    return r.pow(n);
}
template<typename UnscaledValue_t, typename Exponent_t>
BigDecimal<UnscaledValue_t, Exponent_t> round(const BigDecimal<UnscaledValue_t, Exponent_t> &r) noexcept {
    return r.round(RoundingMode::Round);
}
template<typename UnscaledValue_t, typename Exponent_t>
BigDecimal<UnscaledValue_t, Exponent_t> floor(const BigDecimal<UnscaledValue_t, Exponent_t> &r) noexcept {
    return r.round(RoundingMode::Floor);
}
template<typename UnscaledValue_t, typename Exponent_t>
BigDecimal<UnscaledValue_t, Exponent_t> ceil(const BigDecimal<UnscaledValue_t, Exponent_t> &r) noexcept {
    return r.round(RoundingMode::Ceil);
}
template<typename UnscaledValue_t, typename Exponent_t>
BigDecimal<UnscaledValue_t, Exponent_t> trunc(const BigDecimal<UnscaledValue_t, Exponent_t> &r) noexcept {
    return r.round(RoundingMode::Trunc);
}
template<typename UnscaledValue_t, typename Exponent_t>
BigDecimal<UnscaledValue_t, Exponent_t> abs(const BigDecimal<UnscaledValue_t, Exponent_t> &r) noexcept {
    return r.abs();
}
}  // namespace rdf4cpp

#ifndef DOXYGEN_PARSER
template<typename UnscaledValue_t, typename Exponent_t>
struct std::hash<rdf4cpp::BigDecimal<UnscaledValue_t, Exponent_t>> {
    size_t operator()(const rdf4cpp::BigDecimal<UnscaledValue_t, Exponent_t> &r) const {
        return r.hash();
    }
};

template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, ::boost::multiprecision::cpp_int> {
    static size_t dice_hash(::boost::multiprecision::cpp_int const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::hash<::boost::multiprecision::cpp_int>{}(x));
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, ::boost::multiprecision::checked_int128_t> {
    static size_t dice_hash(::boost::multiprecision::checked_int128_t const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::hash<::boost::multiprecision::checked_int128_t>{}(x));
    }
};

template<typename Policy, typename U, typename E>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::BigDecimal<U, E>> {
    static size_t dice_hash(rdf4cpp::BigDecimal<U, E> const &x) noexcept {
        return x.template hash<Policy>();
    }
};
#endif

template<typename UnscaledValue_t, typename Exponent_t>
class std::numeric_limits<rdf4cpp::BigDecimal<UnscaledValue_t, Exponent_t>> {
public:
    using BigDecimal = rdf4cpp::BigDecimal<UnscaledValue_t, Exponent_t>;

    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr std::float_round_style round_style = std::round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = numeric_limits<UnscaledValue_t>::digits;
    static constexpr int digits10 = numeric_limits<UnscaledValue_t>::digits10;
    static constexpr int max_digits10 = numeric_limits<UnscaledValue_t>::max_digits10;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;
    static constexpr BigDecimal max() noexcept {
        return BigDecimal{numeric_limits<UnscaledValue_t>::max(), 0};
    }
    static constexpr BigDecimal min() noexcept {
        return BigDecimal{numeric_limits<UnscaledValue_t>::min(), 0};
    }
    static constexpr BigDecimal lowest() noexcept {
        return min();
    }
    static constexpr BigDecimal epsilon() noexcept {
        return BigDecimal{1, numeric_limits<Exponent_t>::max()};
    }
    static constexpr BigDecimal round_error() noexcept {
        return BigDecimal{0};
    }
    static constexpr BigDecimal infinity() noexcept {
        return 0;
    }
    static constexpr BigDecimal quiet_NaN() noexcept {
        return 0;
    }
    static constexpr BigDecimal signaling_NaN() noexcept {
        return 0;
    }
    static constexpr BigDecimal denorm_min() noexcept {
        return 0;
    }
};

#endif  //RDF4CPP_BIGDECIMAL_H
