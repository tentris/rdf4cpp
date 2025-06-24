#include "IRIView.hpp"

#include <uni_algo/all.h>

#include <rdf4cpp/util/CharMatcher.hpp>

namespace rdf4cpp {
IRIView::IRIView(std::string_view iri) noexcept : data(iri) {
}

std::string_view IRIView::apply(const IRIView::IRIPart &r) const noexcept {
    return data.substr(r.start, r.len);
}
std::optional<std::string_view> IRIView::apply_opt(const IRIView::IRIPart &r) const noexcept {
    if (r.defined)
        return apply(r);
    return std::nullopt;
}

IRIView::IRIPart IRIView::get_scheme_part() const noexcept {
    auto c = data.find(':');
    if (c == std::string_view::npos)
        return {0, 0, false};
    auto s = data.find('/');
    if (c > s)
        return {0, 0, false};
    return {0, c, true};
}
std::optional<std::string_view> IRIView::scheme() const noexcept {
    return apply_opt(get_scheme_part());
}
bool IRIView::is_relative() const noexcept {
    return !get_scheme_part().defined;
}

IRIView::IRIPart IRIView::get_authority_part(const IRIPart &scheme) const noexcept {
    auto d = data;
    auto s = scheme.start + scheme.len;
    if (s > 0) {
        ++s;
        d = d.substr(s);
    }
    if (!d.starts_with("//"))
        return {s, 0, false};
    d = d.substr(2);
    s += 2;
    auto e = d.find_first_of("/#?");
    if (e == std::string_view::npos)
        e = d.length();
    return {s, e, true};
}
std::optional<std::string_view> IRIView::authority() const noexcept {
    return apply_opt(get_authority_part(get_scheme_part()));
}

IRIView::IRIPart IRIView::get_path_part(const IRIPart &auth) const noexcept {
    auto b = auth.start + auth.len;
    auto d = data.substr(b);
    if (auth.len > 0 && !d.starts_with('/'))
        return {b, 0, true};
    auto e = d.find_first_of("#?");
    if (e == std::string_view::npos)
        e = d.length();
    return {b, e, true};
}
std::string_view IRIView::path() const noexcept {
    return apply(get_path_part(get_authority_part(get_scheme_part())));
}

IRIView::IRIPart IRIView::get_query_part(const IRIPart &path) const noexcept {
    auto b = path.start + path.len;
    auto d = data.substr(b);
    if (!d.starts_with('?'))
        return {b, 0, false};
    d = d.substr(1);
    ++b;
    auto e = d.find_first_of('#');
    if (e == std::string_view::npos)
        e = d.length();
    return {b, e, true};
}
std::optional<std::string_view> IRIView::query() const noexcept {
    return apply_opt(get_query_part(get_path_part(get_authority_part(get_scheme_part()))));
}

IRIView::IRIPart IRIView::get_fragment_part(const IRIPart &query) const noexcept {
    auto b = query.start + query.len;
    auto d = data.substr(b);
    if (!d.starts_with('#'))
        return {b, d.length(), false};
    return {b + 1, d.length() - 1, true};
}
std::optional<std::string_view> IRIView::fragment() const noexcept {
    return apply_opt(get_fragment_part(get_query_part(get_path_part(get_authority_part(get_scheme_part())))));
}

IRIView::AllParts IRIView::all_parts() const noexcept {
    auto s = get_scheme_part();
    auto a = get_authority_part(s);
    auto p = get_path_part(a);
    auto q = get_query_part(p);
    auto f = get_fragment_part(q);
    return {apply_opt(s), apply_opt(a), apply(p), apply_opt(q), apply_opt(f)};
}

std::string_view IRIView::to_absolute() const noexcept {
    auto f = get_fragment_part(get_query_part(get_path_part(get_authority_part(get_scheme_part()))));
    if (!f.defined)
        return data;
    return data.substr(0, f.start - 1);
}

IRIView::IRIPart IRIView::get_userinfo_part(const IRIView::IRIPart &auth) const noexcept {
    if (!auth.defined)
        return {0, 0, false};
    auto d = apply(auth);
    auto e = d.find('@');
    if (e == std::string_view::npos)
        return {auth.start, 0, false};
    return {auth.start, e, true};
}
std::optional<std::string_view> IRIView::userinfo() const noexcept {
    return apply_opt(get_userinfo_part(get_authority_part(get_scheme_part())));
}

IRIView::IRIPart IRIView::get_host_part(const IRIView::IRIPart &auth, const IRIView::IRIPart &uinfo) const noexcept {
    if (!auth.defined)
        return {0, 0, false};
    auto d = apply(auth);
    auto b = uinfo.start + uinfo.len;
    if (uinfo.defined) {
        d = d.substr(uinfo.len + 1);
        ++b;
    }
    auto e = d.find_last_of(':');
    if (e == std::string_view::npos || d.find(']', e) != std::string_view::npos)
        return {b, d.length(), true}; // :] or no : at all
    return {b, e, true}; // ]: or :
}
std::optional<std::string_view> IRIView::host() const noexcept {
    auto a = get_authority_part(get_scheme_part());
    return apply_opt(get_host_part(a, get_userinfo_part(a)));
}

IRIView::IRIPart IRIView::get_port_part(const IRIView::IRIPart &auth, const IRIView::IRIPart &host) const noexcept {
    if (!auth.defined)
        return {0, 0, false};
    auto d = apply(auth);
    d = d.substr(host.start - auth.start + host.len);
    auto b = host.start + host.len;
    if (d.starts_with(':'))
        return {b + 1, d.length() - 1, true};
    return {b, 0, false};
}
std::optional<std::string_view> IRIView::port() const noexcept {
    auto a = get_authority_part(get_scheme_part());
    return apply_opt(get_port_part(a, get_host_part(a, get_userinfo_part(a))));
}

IRIView::AuthorityParts IRIView::all_authority_parts() const noexcept {
    auto a = get_authority_part(get_scheme_part());
    auto ui = get_userinfo_part(a);
    auto ho = get_host_part(a, ui);
    auto po = get_port_part(a, ho);
    return {apply_opt(ui), apply_opt(ho), apply_opt(po)};
}

IRIFactoryError IRIView::quick_validate(bool allow_relative) const noexcept {
    using namespace util::char_matcher_detail;
    auto [scheme, auth, path, query, frag] = all_parts();
    if (!scheme.has_value() && !allow_relative)
        return IRIFactoryError::Relative;
    static constexpr auto scheme_pattern = ascii_alphanum_matcher | ASCIIPatternMatcher{"+-."};
    if (scheme.has_value() && !match<scheme_pattern, una::views::utf8>(*scheme))
        return IRIFactoryError::InvalidScheme;
    auto [userinfo, host, port] = all_authority_parts();
    static constexpr auto userinfo_pattern = i_unreserved_matcher | sub_delims_matcher | ASCIIPatternMatcher{"%:"};
    if (userinfo.has_value() && !match<userinfo_pattern, una::views::utf8>(*userinfo))
        return IRIFactoryError::InvalidUserinfo;
    static constexpr auto host_pattern = i_unreserved_matcher | sub_delims_matcher | ASCIIPatternMatcher{"%[]:"};
    if (host.has_value() && !match<host_pattern, una::views::utf8>(*host))
        return IRIFactoryError::InvalidHost;
    static constexpr ASCIINumMatcher port_pattern{};
    if (port.has_value() && !match<port_pattern, una::views::utf8>(*port))
        return IRIFactoryError::InvalidPort;
    static constexpr auto path_pattern = i_unreserved_matcher | sub_delims_matcher | ASCIIPatternMatcher{"%:@/"};
    if (!match<path_pattern, una::views::utf8>(path))
        return IRIFactoryError::InvalidPath;
    static constexpr auto query_pattern = i_unreserved_matcher | sub_delims_matcher | IPrivateMatcher{} | ASCIIPatternMatcher{"%:@/?"};
    if (query.has_value() && !match<query_pattern, una::views::utf8>(*query))
        return IRIFactoryError::InvalidQuery;
    static constexpr auto frag_pattern = i_unreserved_matcher | sub_delims_matcher | ASCIIPatternMatcher{"%:@/?"};
    if (frag.has_value() && !match<frag_pattern, una::views::utf8>(*frag))
        return IRIFactoryError::InvalidFragment;
    return IRIFactoryError::Ok;
}
}
