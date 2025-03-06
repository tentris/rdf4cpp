#ifndef RDF4CPP_LITERALID_HPP
#define RDF4CPP_LITERALID_HPP

#include <cstddef>
#include <cassert>
#include <compare>
#include <cstdint>
#include <ostream>

namespace rdf4cpp::storage::identifier {
/**
 * <p>An LiteralID identifies a Literal of a given type (e.g. xsd:integer, xsd:string, etc.) within a given NodeStorage.</p>
 * <p>LiteralIDs are available in the range [0,(2^42-1)].</P>
 */
struct __attribute__((__packed__)) LiteralID {
    static constexpr size_t width = 42;
    using underlying_type = uint64_t;

private:
    underlying_type underlying: width;

public:
    constexpr LiteralID() noexcept = default;

    /**
     * Constructor
     * @param underlying literal ID. MUST be smaller than 2^42. Bounds are not checked.
     */
    explicit constexpr LiteralID(underlying_type const underlying) noexcept : underlying{underlying} {
        assert(underlying < (1UL << 42));
    }

    constexpr LiteralID &operator++() noexcept {
        ++underlying;
        return *this;
    }

    [[nodiscard]] constexpr underlying_type to_underlying() const noexcept {
        return underlying;
    }

    explicit operator underlying_type() const noexcept {
        return underlying;
    }

    constexpr LiteralID operator++(int) noexcept {
        LiteralID new_literal_id{*this};
        ++underlying;
        return new_literal_id;
    }

    constexpr LiteralID &operator--() noexcept {
        --underlying;
        return *this;
    }

    constexpr LiteralID operator--(int) noexcept {
        LiteralID new_literal_id{*this};
        --underlying;
        return new_literal_id;
    }

    constexpr std::strong_ordering operator<=>(LiteralID const &) const noexcept = default;
};

inline std::ostream &operator<<(std::ostream &os, LiteralID id) {
    os << "LiteralID { .underlying = " << id.to_underlying() << " }";
    return os;
}

}  // namespace rdf4cpp::storage::identifier
#endif  //RDF4CPP_LITERALID_HPP
