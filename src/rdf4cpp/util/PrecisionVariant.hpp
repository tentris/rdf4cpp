#ifndef RDF4CPP_PRECISIONVARIANT_HPP
#define RDF4CPP_PRECISIONVARIANT_HPP

#include <boost/container_hash/hash.hpp>
#include <rdf4cpp/Int128.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>
#include <rdf4cpp/util/boost_int.hpp>

#include <dice/template-library/variant2.hpp>

namespace rdf4cpp::util {
    template<typename Small, typename Big>
    struct PrecisionVariant : dice::template_library::variant2<Small, Big> {

        using small_type = Small;
        using big_type = Big;

        static PrecisionVariant parse(std::string_view s, auto f) {
            try {
                return PrecisionVariant{f.template operator()<Small>(s)};
            }
            catch (InvalidNode const&) {
                return PrecisionVariant{f.template operator()<Big>(s)};
            }
        }

        template<typename F>
        std::expected<PrecisionVariant, datatypes::DynamicError> apply_op(PrecisionVariant const& other, F f) const noexcept {
            namespace dtl = dice::template_library;

            auto call = [&]<typename T>(const T& a, const T& b, T& r) {
                if constexpr (requires (const T& a, const T& b, F f) { f(a, b); }) {
                    auto v = f(a, b);
                    if (v.has_value()) {
                        r = *v;
                        return false;
                    }
                    return true;
                }
                else {
                    return f(a, b, r);
                }
            };

            if (dtl::holds_alternative<Small>(*this) && dtl::holds_alternative<Small>(other)) {
                Small r;
                if (!call(dtl::get<Small>(*this), dtl::get<Small>(other), r)) {
                    return PrecisionVariant{r};
                }
            }

            Big this_conv;
            Big other_conv;

            static constexpr auto convert = [](PrecisionVariant const& in, Big& storage) {
                return dtl::visit([&]<typename C>(C const& c) -> Big const& {
                    if constexpr (std::is_same_v<C, Big>) {
                        return c;
                    }
                    else {
                        storage = Big{c};
                        return storage;
                    }
                }, in);
            };

            Big r;
            if (call(convert(*this, this_conv), convert(other, other_conv), r)) [[unlikely]] {
                return std::unexpected(datatypes::DynamicError::OverOrUnderFlow);
            }
            return PrecisionVariant{r};
        }

        auto operator<=>(int z) const noexcept {
            return dice::template_library::visit([&]<typename T>(const T& v) {
                return v <=> T{z};
            }, *this);
        }
        bool operator==(int z) const noexcept {
            return dice::template_library::visit([&]<typename T>(const T& v) {
               return v == T{z};
           }, *this);
        }
        auto operator<=>(const PrecisionVariant & other) const {
            namespace dtl = dice::template_library;

            if (dtl::holds_alternative<Small>(*this) && dtl::holds_alternative<Small>(other)) {
                return dtl::get<Small>(*this) <=> dtl::get<Small>(other);
            }
            if (dtl::holds_alternative<Big>(*this) && dtl::holds_alternative<Big>(other)) {
                return dtl::get<Big>(*this) <=> dtl::get<Big>(other);
            }

            Big this_conv;
            Big other_conv;

            static constexpr auto convert = [](PrecisionVariant const& in, Big& storage) {
                return dtl::visit([&]<typename C>(C const& c) -> Big const& {
                    if constexpr (std::is_same_v<C, Big>) {
                        return c;
                    }
                    else {
                        storage = Big{c};
                        return storage;
                    }
                }, in);
            };

            return convert(*this, this_conv) <=> convert(other, other_conv);
        }

        [[nodiscard]] size_t hash() const noexcept {
            auto idx = this->index();
            auto contained = dice::template_library::visit([&]<typename T>(const T& v) {
                return std::hash(v);
            });
            return boost::hash_combine((idx, contained));
        }

        template<dice::hash::Policies::HashPolicy Policy = dice::hash::Policies::wyhash>
        [[nodiscard]] size_t hash() const {
            auto idx = this->index();
            return dice::template_library::visit([&]<typename T>(const T& v) {
                return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(idx, v));
            });
        }
    };
}

#ifndef DOXYGEN_PARSER
template<typename Small, typename Big>
struct std::hash<rdf4cpp::util::PrecisionVariant<Small, Big>> {
    size_t operator()(rdf4cpp::util::PrecisionVariant<Small, Big> const &r) const {
        return r.hash();
    }
};

template<typename Policy, typename Small, typename Big>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::util::PrecisionVariant<Small, Big>> {
    static size_t dice_hash(rdf4cpp::util::PrecisionVariant<Small, Big> const &x) noexcept {
        return x.template hash<Policy>();
    }
};
#endif

#endif // RDF4CPP_PRECISIONVARIANT_HPP
