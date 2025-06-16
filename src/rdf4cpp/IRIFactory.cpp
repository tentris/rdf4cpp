#include "IRIFactory.hpp"

#include <rdf4cpp/datatypes/registry/DatatypeRegistry.hpp>
#include <rdf4cpp/util/CharMatcher.hpp>

#include <uni_algo/all.h>

namespace rdf4cpp {

/**
* turns the parts of a IRI back into a full IRI.
*/
static std::string_view construct(std::string_view scheme, std::optional<std::string_view> auth, std::string_view path,
                                  std::optional<std::string_view> query, std::optional<std::string_view> frag) noexcept {
    static thread_local std::string str;
    str.clear();
    str.reserve(std::bit_ceil(scheme.size() + 1 + path.size()));
    str.append(scheme);
    str.push_back(':');

    if (auth.has_value()) {
        str.append("//");
        str.append(*auth);
    }

    if (!path.empty() && !path.starts_with('/') && auth.has_value()) {
        str.push_back('/');
    }

    str.append(path);

    if (query.has_value()) {
        str.push_back('?');
        str.append(*query);
    }

    if (frag.has_value()) {
        str.push_back('#');
        str.append(*frag);
    }

    return str;
}

/**
* gets the first segment of path
*/
static std::string_view first_path_segment(std::string_view path) noexcept {
    size_t off = 0;
    if (path.starts_with('/'))
        off = 1;
    auto e = path.find('/', off);
    return path.substr(0, e);
}

/**
 * removes the last segment of path
 */
static void remove_last_path_segment(std::string &path) noexcept {
    auto e = path.find_last_of('/');
    if (e == std::string::npos)
        return;
    path.resize(e);
}

/**
* removes ./ and ../ segments from path.
*/
static std::string_view remove_dot_segments(std::string_view src) noexcept {
    // adapted from https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.4

    thread_local static std::string buf;
    buf.clear();
    buf.reserve(std::bit_ceil(src.size()));

    while (!src.empty()) {
        if (src.starts_with("./")) {
            // 2.A
            src.remove_prefix(2);
            continue;
        }

        if (src.starts_with("../")) {
            // 2.A
            src.remove_prefix(3);
            continue;
        }

        if (src.starts_with("/./")) {
            // 2.B
            src.remove_prefix(2);
            continue;
        }

        if (src == "/.") {
            // 2.B
            // '[..] begins with a prefix of [..] "/." where "." is a complete path segment
            // then replace that prefix with "/" in the input buffer [..]' (and continue)

            // => "." is a complete path segment if either a slash follows (previous branch)
            //      or the path ends after it (this branch)
            // => the next iteration would just append the '/' to the output buffer
            //      because none of the branches will be taken
            // => therefore appending the slash directly and breaking out of the loop is equivalent
            //      to the given formulation from RFC 3986
            buf.push_back('/');
            break;
        }

        if (src.starts_with("/../")) {
            // 2.C
            src.remove_prefix(3);
            remove_last_path_segment(buf);
            continue;
        }

        if (src == "/..") {
            // 2.C
            // same reasoning as for 2.B
            remove_last_path_segment(buf);
            buf.push_back('/');
            break;
        }

        if (src == ".." || src == ".") {
            // 2.D
            break;
        }

        // 2.E
        auto const seg = first_path_segment(src);
        buf.append(seg);
        src.remove_prefix(seg.size());
    }

    return buf;
}

/**
 * merges the path of the current base and path, as described in https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.3.
 */
static std::string_view merge_path_with_base(IRIView::AllParts const &base, std::string_view path) noexcept {
    static thread_local std::string r;
    r.clear();

    if (base.scheme.has_value() && base.path.empty()) {
        r.reserve(std::bit_ceil(path.size() + 1));
        r.push_back('/');
        r.append(path);
        return r;
    }

    r.reserve(std::bit_ceil(base.path.size() + path.size() + 1));
    r.append(base.path);
    remove_last_path_segment(r);
    r.push_back('/');
    r.append(path);
    return r;
}

/**
 * Converts rel to an absolute IRI by merging it with the given base
 */
template<bool always_remove_dots>
static std::string_view to_absolute(IRIView::AllParts const &base, std::string_view rel) noexcept {
    auto [r_scheme, r_auth, r_path, r_query, r_frag] = IRIView{rel}.all_parts();

    if (r_scheme.has_value()) {
        if constexpr(always_remove_dots) {
            return construct(*r_scheme, r_auth, remove_dot_segments(r_path), r_query, r_frag);
        }
        else {
            return rel;
        }
    }

    auto const &[b_scheme, b_auth, b_path, b_query, _b_frag] = base;

    if (r_auth.has_value()) {
        return construct(*b_scheme, r_auth, remove_dot_segments(r_path), r_query, r_frag);
    }
    if (r_path.empty()) {
        return construct(*b_scheme, b_auth, b_path, r_query.has_value() ? r_query : b_query, r_frag);
    }
    if (r_path.starts_with('/')) {
        return construct(*b_scheme, b_auth, remove_dot_segments(r_path), r_query, r_frag);
    }

    auto const merged = merge_path_with_base(base, r_path);
    return construct(*b_scheme, b_auth, remove_dot_segments(merged), r_query, r_frag);
}


IRIFactory::IRIFactory(std::string_view base) {
    if (set_base(base) != IRIFactoryError::Ok) {
        throw std::invalid_argument{"invalid base"};
    }
}

IRIFactory::IRIFactory(prefix_map_type &&prefixes, std::string_view base) : prefixes(std::move(prefixes)) {
    if (set_base(base) != IRIFactoryError::Ok) {
        throw std::invalid_argument{"invalid base"};
    }
}

nonstd::expected<IRI, IRIFactoryError> IRIFactory::from_relative(std::string_view rel, storage::DynNodeStoragePtr node_storage) const noexcept {
    return create_and_validate(to_absolute<true>(base_parts_cache, rel), node_storage);
}

nonstd::expected<IRI, IRIFactoryError> IRIFactory::from_maybe_relative(std::string_view rel, storage::DynNodeStoragePtr node_storage) const noexcept {
    return create_and_validate(to_absolute<false>(base_parts_cache, rel), node_storage);
}

nonstd::expected<IRI, IRIFactoryError> IRIFactory::from_prefix(std::string_view prefix, std::string_view local, storage::DynNodeStoragePtr node_storage) const {
    auto i = prefixes.find(prefix);
    if (i == prefixes.end()) {
        return nonstd::make_unexpected(IRIFactoryError::UnknownPrefix);
    }

    static thread_local std::string deref;
    deref.clear();
    deref.reserve(i->second.size() + local.size());
    deref.append(i->second);
    deref.append(local);

    if (IRIView{deref}.is_relative()) {
        return from_relative(deref, node_storage);
    }

    return create_and_validate(deref, node_storage);
}

nonstd::expected<IRI, IRIFactoryError> IRIFactory::create_and_validate(std::string_view iri, storage::DynNodeStoragePtr node_storage) noexcept {
    if (!rdf4cpp::datatypes::registry::relaxed_parsing_mode) {
        if (auto const e = IRIView{iri}.quick_validate(); e != IRIFactoryError::Ok) {
            return nonstd::make_unexpected(e);
        }
    }
    return IRI::make_unchecked(iri, node_storage);
}

IRIFactoryError IRIFactory::assign_prefix(std::string_view prefix, std::string_view expanded) {
    using namespace util::char_matcher_detail;
    auto r = prefix | una::views::utf8;
    auto it = r.begin();
    if (it != r.end()) {
        if (!PNCharsBaseMatcher.match(*it)) {
            return IRIFactoryError::InvalidPrefix;
        }
        auto lastchar = *it;
        ++it;
        static constexpr auto pn_matcher = PNCharsMatcher | ASCIIPatternMatcher{"."};
        while (it != r.end()) {
            if (!pn_matcher.match(*it)) {
                return IRIFactoryError::InvalidPrefix;
            }
            lastchar = *it;
            ++it;
        }
        if (lastchar == '.') {
            return IRIFactoryError::InvalidPrefix;
        }
    }
    // checking expanded can only be done after the full IRI was created
    assign_prefix_unchecked(prefix, expanded);
    return IRIFactoryError::Ok;
}
void IRIFactory::assign_prefix_unchecked(std::string_view prefix, std::string_view expanded) {
    std::string pre{prefix};
    prefixes[pre] = expanded;
}
void IRIFactory::clear_prefix(std::string_view prefix) {
    auto it = prefixes.find(prefix);
    if (it == prefixes.end()) [[unlikely]] {
        return;
    }

    prefixes.erase(it);
}

std::string_view IRIFactory::get_base() const noexcept {
    return base;
}

IRIFactoryError IRIFactory::set_base(std::string_view b) noexcept {
    if (!rdf4cpp::datatypes::registry::relaxed_parsing_mode) {
        if (auto const e = IRIView{b}.quick_validate(); e != IRIFactoryError::Ok) {
            return e;
        }
    }
    base = b;
    base_parts_cache = IRIView{base}.all_parts();
    return IRIFactoryError::Ok;
}

}  // namespace rdf4cpp
