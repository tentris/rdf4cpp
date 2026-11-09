#ifndef RDF4CPP_TRANSPARENTSTRINGHASH_HPP
#define RDF4CPP_TRANSPARENTSTRINGHASH_HPP

#include <dice/hash.hpp>

#include <string_view>

namespace rdf4cpp::util {

/**
 * Hash for string keys that also accepts a `std::string_view`.
 * A hash map with string keys can then look up a `std::string_view` without building a key.
 * Use it together with `std::equal_to<>`.
 * The hash is wyhash, so it is avalanching (inherits `is_avalanching` from `DiceHash`).
 */
struct TransparentStringHash : dice::hash::DiceHashwyhash<std::string_view> {
    using is_transparent = void;
};

}  // namespace rdf4cpp::util

#endif  // RDF4CPP_TRANSPARENTSTRINGHASH_HPP
