#ifndef RDF4CPP_BIGDECIMAL_H
#define RDF4CPP_BIGDECIMAL_H

#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

#include <boost/charconv.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <dice/hash.hpp>

#include <rdf4cpp/Assert.hpp>
#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/Int128.hpp>
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/boost_int.hpp>

namespace rdf4cpp {
    namespace util {
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

        template<BigDecimalBaseType UnscaledValue_t = Int128, BigDecimalBaseType Exponent_t = uint32_t>
        requires(!std::signed_integral<Exponent_t> && !std::unsigned_integral<UnscaledValue_t>)
        struct BigDecimal {
            // the entire class is loosely based on OpenJDKs BigDecimal: https://github.com/AdoptOpenJDK/openjdk-jdk11/blob/master/src/java.base/share/classes/java/math/BigDecimal.java

        private:
            UnscaledValue_t unscaled_value;
            Exponent_t exponent;

            using OverflowMode = detail::OverflowMode;

            static constexpr uint32_t base = 10;

            static constexpr UnscaledValue_t abs(UnscaledValue_t const &value) noexcept {
                if constexpr (UnsignedIntegralExt<UnscaledValue_t>) {
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
            constexpr BigDecimal(UnscaledValue_t const &unscaled_value, Exponent_t exponent) noexcept
                : unscaled_value(unscaled_value), exponent(exponent) {
            }

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
                bool overflow_decimal = false;
                for (char const c : value) {
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
                    if (overflow_decimal) {
                        if (c != '0') {
                            throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: unscaled_value overflow"};
                        }
                        continue;
                    }
                    if (c < '0' || c > '9') {
                        throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: non-numeric char found"};
                    }
                    auto n = c - '0';
                    UnscaledValue_t next_unscaled;
                    if (detail::mul_checked<OverflowMode::Checked>(unscaled_value, UnscaledValue_t{base}, next_unscaled)
                        || detail::add_checked<OverflowMode::Checked>(next_unscaled, UnscaledValue_t{n}, next_unscaled)) {
                        if (decimal && n == 0) {
                            overflow_decimal = true;
                            continue;
                        }
                        throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: unscaled_value overflow"};
                    }
                    if (decimal) {
                        Exponent_t next_exponent;
                        if (detail::add_checked<OverflowMode::Checked>(exponent, Exponent_t{1}, next_exponent)) {
                            throw InvalidNode{"http://www.w3.org/2001/XMLSchema#decimal parsing error: exponent overflow"};
                        }
                        exponent = next_exponent;
                    }
                    unscaled_value = next_unscaled;
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
            constexpr explicit BigDecimal(uint32_t value) noexcept : BigDecimal(UnscaledValue_t{value}, 0) {
            }

            /**
             * converts a uint64_t to a BigDecimal
             * @param value
             */
            constexpr explicit BigDecimal(uint64_t value) noexcept : BigDecimal(UnscaledValue_t{value}, 0) {
            }

            /**
             * converts a int32_t to a BigDecimal
             * @param value
             */
            constexpr explicit BigDecimal(int32_t value) noexcept : BigDecimal(static_cast<UnscaledValue_t>(value), 0) {
            }

            /**
             * converts a int64_t to a BigDecimal
             * @param value
             */
            constexpr explicit BigDecimal(int64_t value) noexcept : BigDecimal(static_cast<UnscaledValue_t>(value), 0) {
            }

            /**
             * converts a UnscaledValue_t to a BigDecimal
             * @param value
             */
            constexpr explicit BigDecimal(UnscaledValue_t const &value) noexcept
            requires(!std::is_same_v<UnscaledValue_t, int32_t> && !std::is_same_v<UnscaledValue_t, int64_t>)
                : BigDecimal(value, 0) {
            }

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
                while ((significand & 1) == 0) {
                    significand >>= 1;
                    ++ex;
                }
                static constexpr char const *exc = "double to BigDecimal overflow";
                if (ex == 0) {
                    unscaled_value = UnscaledValue_t{significand};
                } else if (ex < 0) {
                    exponent = static_cast<Exponent_t>(-ex);
                    UnscaledValue_t e{0};
                    if (detail::pow_checked<OverflowMode::Checked>(UnscaledValue_t{5}, -ex, e))
                        throw std::overflow_error{exc};
                    if (detail::mul_checked<OverflowMode::Checked>(UnscaledValue_t{significand}, e, unscaled_value))
                        throw std::overflow_error{exc};
                } else {
                    UnscaledValue_t e{0};
                    if (detail::pow_checked<OverflowMode::Checked>(UnscaledValue_t{2}, ex, e))
                        throw std::overflow_error{exc};
                    if (detail::mul_checked<OverflowMode::Checked>(UnscaledValue_t{significand}, e, unscaled_value))
                        throw std::overflow_error{exc};
                }
                if (neg)
                    unscaled_value = -unscaled_value;
                normalize();
            }

            template<std::floating_point F>
            void from_floating(F value) {
                if constexpr (!std::numeric_limits<UnscaledValue_t>::is_bounded) {
                    from_double_direct(value);
                } else {
                    if (std::isinf(value) || std::isnan(value))
                        throw std::invalid_argument{"value is NaN or infinity"};
                    // use the shortest representation that round-trips (<= 17 significant digits) instead of the exact one,
                    // as the exact one would use up all available digits and make the result useless for further arithmetic
                    std::array<char, 350> buf;  // fixed notation of the smallest denormal double needs 326 chars
                    auto const res = boost::charconv::to_chars(buf.data(), buf.data() + buf.size(), value, boost::charconv::chars_format::fixed);
                    RDF4CPP_ASSERT(res.ec == std::errc{});
                    try {
                        *this = BigDecimal{std::string_view{buf.data(), res.ptr}};
                    } catch (InvalidNode const &) {
                        throw std::overflow_error{"double -> decimal overflow"};
                    }
                }
            }

        public:
            /**
             * converts a float/double to a BigDecimal.
             * this conversion might not be exact, due to the built in limitations of floating point numbers.
             * if you have the possibility, use one of the other constructors.
             * @param value
             * @throw std::overflow_error on exceeding the types numeric limits
             * @throw std::invalid_argument on NaN or infinity
             */
            explicit BigDecimal(float value) : unscaled_value(0), exponent(0) {
                from_floating(value);
            }
            explicit BigDecimal(double value) : unscaled_value(0), exponent(0) {
                from_floating(value);
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
                if (unscaled_value == 0) {
                    exponent = 0;
                    return;
                }
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
            template<OverflowMode m, bool (*op_checked)(UnscaledValue_t const &t, UnscaledValue_t const &o, UnscaledValue_t &result)>
            constexpr bool add_or_sub(BigDecimal const &other, BigDecimal &result) const noexcept {
                UnscaledValue_t t = this->unscaled_value;
                UnscaledValue_t o = other.unscaled_value;
                Exponent_t new_exp = std::max(this->exponent, other.exponent);
                if (this->exponent < new_exp) {
                    UnscaledValue_t ex{0};
                    if (detail::pow_checked<m>(UnscaledValue_t{base}, new_exp - this->exponent, ex))
                        return true;
                    if (detail::mul_checked<m>(t, ex, t))
                        return true;
                } else if (other.exponent < new_exp) {
                    UnscaledValue_t ex{0};
                    if (detail::pow_checked<m>(UnscaledValue_t{base}, new_exp - other.exponent, ex))
                        return true;
                    if (detail::mul_checked<m>(o, ex, o))
                        return true;
                }
                UnscaledValue_t res = 0;
                if (op_checked(t, o, res))
                    return true;
                normalize(res, new_exp);
                result = BigDecimal{res, new_exp};
                return false;
            }

            template<OverflowMode m>
            constexpr bool mul(BigDecimal const &other, BigDecimal &result) const noexcept {
                UnscaledValue_t v{0};
                if (detail::mul_checked<m>(this->unscaled_value, other.unscaled_value, v))
                    return true;
                Exponent_t e{0};
                if (detail::add_checked<m>(this->exponent, other.exponent, e))
                    return true;
                normalize(v, e);
                result = BigDecimal{v, e};
                return false;
            }

            // whether a truncated quotient (neg: its sign) with a non-zero remainder (half: |rem / div| >= 0.5) is moved away from zero by rounding mode m
            static constexpr bool round_away_from_zero(bool neg, bool half, RoundingMode m) noexcept {
                return m == RoundingMode::Round ? half : m == RoundingMode::Floor ? neg : m == RoundingMode::Ceil && !neg;
            }

            // rounds the truncated quotient v (of a division with remainder rem by div) according to mode
            template<OverflowMode m>
            static constexpr bool handle_rounding(UnscaledValue_t v, Exponent_t e, UnscaledValue_t const &rem, UnscaledValue_t const &div, RoundingMode mode, BigDecimal &result) noexcept {
                if (rem != 0) {
                    bool const neg = (rem < 0) != (div < 0);
                    bool const half = abs(rem) >= abs(div / 2) + abs(div % 2);  // overflow free
                    if (round_away_from_zero(neg, half, mode) && detail::add_checked<m>(v, UnscaledValue_t{neg ? -1 : 1}, v))
                        return true;
                }
                result = BigDecimal{v, e};
                return false;
            }

            template<OverflowMode m>
            constexpr bool div(BigDecimal const &other, Exponent_t max_scale_increase, RoundingMode mode, BigDecimal &result) const noexcept {
                if (this->unscaled_value == 0) {
                    result = BigDecimal{0, 0};
                    return false;
                }
                UnscaledValue_t t = this->unscaled_value;
                Exponent_t ex = this->exponent;
                UnscaledValue_t div = other.unscaled_value;
                if (ex >= other.exponent) {
                    if (detail::sub_checked<m>(ex, other.exponent, ex))
                        return true;
                } else {
                    UnscaledValue_t scale{0};
                    if (detail::pow_checked<m>(UnscaledValue_t{base}, other.exponent - ex, scale) || detail::mul_checked<m>(t, scale, t))
                        return true;
                    ex = 0;
                }
                if constexpr (m == OverflowMode::Checked) {
                    if (t == std::numeric_limits<UnscaledValue_t>::min() && div == -1) {
                        return true;
                    }
                }
                bool const neg = (t < 0) != (div < 0);
                UnscaledValue_t res = t / div;
                UnscaledValue_t rem = t % div;  // keeps the sign of t
                while (rem != 0 && max_scale_increase > 0) {
                    if constexpr (IntegralExt<Exponent_t>) {
                        if (ex == std::numeric_limits<Exponent_t>::max())
                            break;
                    }
                    // next digit (|rem| * base) / |div| and remainder (|rem| * base) % |div| of the long division,
                    // computed by repeated addition, because |rem| * base (or even |div|) might not fit
                    UnscaledValue_t const r0 = abs(rem);
                    UnscaledValue_t const step = abs(div < 0 ? div + r0 : div - r0);  // |div| - |rem|
                    UnscaledValue_t r = r0;
                    UnscaledValue_t digit = 0;
                    for (uint32_t i = 1; i < base; ++i) {
                        if (r >= step) {  // r + r0 >= |div|
                            r -= step;
                            ++digit;
                        } else {
                            r += r0;
                        }
                    }
                    // stop adding digits (and round) once the next one does not fit anymore
                    UnscaledValue_t next_res;
                    if (detail::mul_checked<OverflowMode::Checked>(res, UnscaledValue_t{base}, next_res)
                        || detail::add_checked<OverflowMode::Checked>(next_res, neg ? -digit : digit, next_res))
                        break;
                    ++ex;
                    res = next_res;
                    rem = rem < 0 ? -r : r;
                    --max_scale_increase;
                }
                return handle_rounding<m>(res, ex, rem, div, mode, result);
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
                if constexpr (IntegralExt<UnscaledValue_t>) {
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
            [[nodiscard]] constexpr BigDecimal operator+(BigDecimal const &other) const noexcept {
                BigDecimal res{0};
                add_or_sub<OverflowMode::UndefinedBehavior, detail::add_checked<OverflowMode::UndefinedBehavior>>(other, res);
                return res;
            }
            /**
             * addition of two BigDecimals.
             * checks overflow.
             * @param other
             * @return
             */
            [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> add_checked(BigDecimal const &other) const noexcept {
                BigDecimal res{0};
                if (add_or_sub<OverflowMode::Checked, detail::add_checked<OverflowMode::Checked>>(other, res))
                    return nonstd::make_unexpected(DecimalError::Overflow);
                return res;
            }

            /**
             * addition of two BigDecimals.
             * may overflow.
             * @param other
             * @return
             */
            constexpr BigDecimal &operator+=(BigDecimal const &other) noexcept {
                *this = *this + other;
                return *this;
            }

            /**
             * subtraction of two BigDecimals.
             * may overflow.
             * @param other
             * @return
             */
            [[nodiscard]] constexpr BigDecimal operator-(BigDecimal const &other) const noexcept {
                BigDecimal res{0};
                add_or_sub<OverflowMode::UndefinedBehavior, detail::sub_checked<OverflowMode::UndefinedBehavior>>(other, res);
                return res;
            }

            /**
             * subtraction of two BigDecimals.
             * checks overflow.
             * @param other
             * @return
             */
            [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> sub_checked(BigDecimal const &other) const noexcept {
                BigDecimal res{0};
                if (add_or_sub<OverflowMode::Checked, detail::sub_checked<OverflowMode::Checked>>(other, res))
                    return nonstd::make_unexpected(DecimalError::Overflow);
                return res;
            }

            /**
             * subtraction of two BigDecimals.
             * may overflow.
             * @param other
             * @return
             */
            constexpr BigDecimal operator-=(BigDecimal const &other) noexcept {
                *this = *this - other;
                return *this;
            }

            /**
             * multiplication of two BigDecimals.
             * may overflow.
             * @param other
             * @return
             */
            [[nodiscard]] constexpr BigDecimal operator*(BigDecimal const &other) const noexcept {
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
            [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> mul_checked(BigDecimal const &other) const noexcept {
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
            constexpr BigDecimal &operator*=(BigDecimal const &other) noexcept {
                *this = *this * other;
                return *this;
            }

            /**
             * default value for scale increase when using operators.
             * when this many decimal places have been added, stop dividing and floor.
             */
            static constexpr Exponent_t default_max_scale_increase = 20;

            /**
             * division of two BigDecimals.
             * may overflow.
             * after default_max_scale_increase decimal places have been added, stops dividing and floor.
             * dividing by 0 is undefined behavior.
             * @param other
             * @return
             */
            [[nodiscard]] constexpr BigDecimal operator/(BigDecimal const &other) const noexcept {
                return div(other, default_max_scale_increase);
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
            [[nodiscard]] constexpr BigDecimal div(BigDecimal const &other, Exponent_t max_scale_increase, RoundingMode mode = RoundingMode::Floor) const noexcept {
                BigDecimal res{0, 0};  // single return object, so NRVO applies (gcc -Werror=nrvo)
                if (other.unscaled_value != 0)  // division by 0 is undefined behavior (cpp_int throws)
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
            [[nodiscard]] constexpr nonstd::expected<BigDecimal, DecimalError> div_checked(BigDecimal const &other, Exponent_t max_scale_increase, RoundingMode mode = RoundingMode::Floor) const noexcept {
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
            constexpr BigDecimal &operator/=(BigDecimal const &other) noexcept {
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
                BigDecimal res{0, 0};
                UnscaledValue_t v{0};
                if (detail::pow_checked<OverflowMode::Checked>(UnscaledValue_t{base}, exponent, v)) {
                    // base^exponent does not fit while unscaled_value does, so |*this| < 1 and the result is 0 or +-1.
                    // |*this| >= 0.5 iff |unscaled_value| >= 5 * base^(exponent - 1)
                    bool const half = !detail::pow_checked<OverflowMode::Checked>(UnscaledValue_t{base}, exponent - 1, v)
                                      && !detail::mul_checked<OverflowMode::Checked>(v, UnscaledValue_t{5}, v)
                                      && (unscaled_value >= v || unscaled_value <= -v);
                    if (unscaled_value != 0 && round_away_from_zero(!positive(), half, mode))
                        res = BigDecimal{positive() ? 1 : -1, 0};
                } else {
                    handle_rounding<OverflowMode::UndefinedBehavior>(unscaled_value / v, 0, unscaled_value % v, v, mode, res);
                }
                return res;
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
            constexpr std::strong_ordering operator<=>(BigDecimal const &other) const noexcept {
                if (this->positive() != other.positive())
                    return this->positive() ? std::strong_ordering::greater : std::strong_ordering::less;
                UnscaledValue_t t = this->unscaled_value;
                UnscaledValue_t o = other.unscaled_value;
                // scale both values to have the same exponent
                // if one of the decimals is 0, skip and compare directly (0*base^e is 0 for all e)
                if (t != 0 && o != 0) {
                    if (this->exponent > other.exponent) {
                        UnscaledValue_t b{base};
                        if (detail::pow_checked<OverflowMode::Checked>(b, this->exponent - other.exponent, b))
                            return this->positive() ? std::strong_ordering::less : std::strong_ordering::greater;  // t does fit into the same precision, while o does not
                        if (detail::mul_checked<OverflowMode::Checked>(o, b, o))
                            return this->positive() ? std::strong_ordering::less : std::strong_ordering::greater;  // t does fit into the same precision, while o does not
                    } else if (this->exponent < other.exponent) {
                        UnscaledValue_t b{base};
                        if (detail::pow_checked<OverflowMode::Checked>(b, other.exponent - this->exponent, b))
                            return this->positive() ? std::strong_ordering::greater : std::strong_ordering::less;  // o does fit into the same precision, while t does not
                        if (detail::mul_checked<OverflowMode::Checked>(t, b, t))
                            return this->positive() ? std::strong_ordering::greater : std::strong_ordering::less;  // o does fit into the same precision, while t does not
                    }
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
            constexpr bool operator==(BigDecimal const &other) const noexcept {
                return (*this <=> other) == std::strong_ordering::equivalent;
            }
            /**
             * equality between a BigDecimal and a int (mainly for constants)
             * @param t
             * @param other
             * @return
             */
            friend bool operator==(BigDecimal const &t, int other) noexcept {
                return t == BigDecimal{other, 0};
            }
            /**
             * equality between a BigDecimal and a int (mainly for constants)
             * @param t
             * @param other
             * @return
             */
            friend bool operator==(int t, BigDecimal const &other) noexcept {
                return other == t;
            }

            /**
             * conversion to a double
             * @return
             */
            [[nodiscard]] explicit operator double() const noexcept {
                double const v = static_cast<double>(unscaled_value) * std::pow(static_cast<double>(base), -static_cast<double>(exponent));
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
            friend std::ostream &operator<<(std::ostream &str, BigDecimal const &bn) {
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
        std::string to_string(BigDecimal<UnscaledValue_t, Exponent_t> const &r) noexcept {
            return static_cast<std::string>(r);
        }

        template<typename UnscaledValue_t, typename Exponent_t>
        BigDecimal<UnscaledValue_t, Exponent_t> pow(BigDecimal<UnscaledValue_t, Exponent_t> const &r, unsigned int n) noexcept {
            return r.pow(n);
        }
        template<typename UnscaledValue_t, typename Exponent_t>
        BigDecimal<UnscaledValue_t, Exponent_t> round(BigDecimal<UnscaledValue_t, Exponent_t> const &r) noexcept {
            return r.round(RoundingMode::Round);
        }
        template<typename UnscaledValue_t, typename Exponent_t>
        BigDecimal<UnscaledValue_t, Exponent_t> floor(BigDecimal<UnscaledValue_t, Exponent_t> const &r) noexcept {
            return r.round(RoundingMode::Floor);
        }
        template<typename UnscaledValue_t, typename Exponent_t>
        BigDecimal<UnscaledValue_t, Exponent_t> ceil(BigDecimal<UnscaledValue_t, Exponent_t> const &r) noexcept {
            return r.round(RoundingMode::Ceil);
        }
        template<typename UnscaledValue_t, typename Exponent_t>
        BigDecimal<UnscaledValue_t, Exponent_t> trunc(BigDecimal<UnscaledValue_t, Exponent_t> const &r) noexcept {
            return r.round(RoundingMode::Trunc);
        }
        template<typename UnscaledValue_t, typename Exponent_t>
        BigDecimal<UnscaledValue_t, Exponent_t> abs(BigDecimal<UnscaledValue_t, Exponent_t> const &r) noexcept {
            return r.abs();
        }
    }  // namespace util

    using Decimal128 = util::BigDecimal<>;
}  // namespace rdf4cpp

#ifndef DOXYGEN_PARSER
template<typename UnscaledValue_t, typename Exponent_t>
struct std::hash<rdf4cpp::util::BigDecimal<UnscaledValue_t, Exponent_t>> {
    size_t operator()(rdf4cpp::util::BigDecimal<UnscaledValue_t, Exponent_t> const &r) const {
        return r.hash();
    }
};

template<typename Policy, typename U, typename E>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::util::BigDecimal<U, E>> {
    static size_t dice_hash(rdf4cpp::util::BigDecimal<U, E> const &x) noexcept {
        return x.template hash<Policy>();
    }
};
#endif

template<typename UnscaledValue_t, typename Exponent_t>
class std::numeric_limits<rdf4cpp::util::BigDecimal<UnscaledValue_t, Exponent_t>> {
public:
    using BigDecimal = rdf4cpp::util::BigDecimal<UnscaledValue_t, Exponent_t>;

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
