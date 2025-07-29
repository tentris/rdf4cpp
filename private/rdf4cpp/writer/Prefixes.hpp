#ifndef RDF4CPP_PRIVATE_PREFIXES_HPP
#define RDF4CPP_PRIVATE_PREFIXES_HPP

#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::writer {

struct TypeIRIPrefix {
    std::string_view prefix;
    std::string_view shorthand;
};

inline constexpr std::array iri_prefixes{
        TypeIRIPrefix{"http://www.w3.org/2001/XMLSchema#", "xsd"},
        TypeIRIPrefix{"http://www.w3.org/1999/02/22-rdf-syntax-ns#", "rdf"},
};

static consteval typename decltype(iri_prefixes)::const_iterator find_prefix(std::string_view name) {
    return std::ranges::find_if(iri_prefixes, [&](auto const &pre) {
        return name.starts_with(pre.prefix);
    });
}

namespace detail {

template<size_t N, size_t Ix, size_t Len>
static consteval std::array<std::string_view, N> make_type_iri_buf(std::array<std::string_view, N> ret = {}) {
    using namespace datatypes::registry;
    using namespace datatypes::registry::util;

    if constexpr (Ix >= Len) {
        return ret;
    } else {
        if constexpr (constexpr auto it = find_prefix(reserved_datatype_ids.storage[Ix].first); it != writer::iri_prefixes.end()) {
            constexpr auto without_prefix = reserved_datatype_ids.storage[Ix].first.substr(it->prefix.size());

            using concat = ConstexprStringHolder<ConstexprString<it->shorthand.size() + 1>{it->shorthand}
                                                 + ConstexprString{":"}
                                                 + ConstexprString<without_prefix.size() + 1>{without_prefix}>;

            ret[static_cast<size_t>(reserved_datatype_ids.storage[Ix].second.to_underlying())] = concat::value;
        }

        return make_type_iri_buf<N, Ix + 1, Len>(ret);
    }
}

static constexpr auto type_iri_buffer = make_type_iri_buf<1 << storage::identifier::LiteralType::width, 0, datatypes::registry::reserved_datatype_ids.size()>();

} // namespace detail

// will only get called with fixed ids
inline bool write_fixed_type_iri(datatypes::registry::LiteralType t, BufWriterParts const writer, bool use_prefixes) noexcept {
    RDF4CPP_ASSERT(t.is_fixed());

    if (use_prefixes) {
        auto const &p = detail::type_iri_buffer[t.to_underlying()];
        if (!p.empty()) {
            return write_str(p, writer);
        }
    }

    RDF4CPP_DETAIL_TRY_WRITE_STR("<");
    RDF4CPP_DETAIL_TRY_WRITE_STR(datatypes::registry::DatatypeRegistry::get_entry(t)->datatype_iri);
    return write_str(">", writer);
}

} // namespace rdf4cpp::writer

#endif // RDF4CPP_PRIVATE_PREFIXES_HPP
