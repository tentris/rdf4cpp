#ifndef RDF4CPP_RDF_REGEX_REGEXFLAGS_HPP
#define RDF4CPP_RDF_REGEX_REGEXFLAGS_HPP

#include <cstdint>
#include <type_traits>

namespace rdf4cpp::regex {

enum struct RegexFlag : uint8_t {
    DotAll = 1 << 0,
    CaseInsensitive = 1 << 1,
    Literal = 1 << 2,
    Multiline = 1 << 3,
    RemoveWhitespace = 1 << 4,
    Optimize = 1 << 5,
};

struct RegexFlags {
private:
    using flag_u_type = std::underlying_type_t<RegexFlag>;
    flag_u_type flags;

    constexpr RegexFlags(uint8_t const flags) noexcept : flags{flags} {}

public:
    constexpr RegexFlags(RegexFlag const flag) noexcept
        : flags{static_cast<flag_u_type>(flag)} {
    }

    static constexpr RegexFlags none() noexcept {
        return RegexFlags{0};
    }

    [[nodiscard]] constexpr bool contains(RegexFlag const flag) const noexcept {
        return flags & static_cast<flag_u_type>(flag);
    }

    constexpr RegexFlags &operator|=(RegexFlags const other) noexcept {
        this->flags |= other.flags;
        return *this;
    }

    constexpr RegexFlags operator|(RegexFlags const other) const noexcept {
        auto cpy = *this;
        return (cpy |= other);
    }

    constexpr RegexFlags &operator|=(RegexFlag const flag) noexcept {
        this->flags |= static_cast<flag_u_type>(flag);
        return *this;
    }

    constexpr RegexFlags operator|(RegexFlag const flag) const noexcept {
        auto cpy = *this;
        return (cpy |= flag);
    }
};

constexpr RegexFlags operator|(RegexFlag const f1, RegexFlag const f2) noexcept {
    return RegexFlags::none() | f1 | f2;
}

}  //namespace rdf4cpp::regex

#endif  //RDF4CPP_RDF_REGEX_REGEXFLAGS_HPP
