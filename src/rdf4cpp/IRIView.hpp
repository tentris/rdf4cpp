#ifndef RDF4CPP_IRIVIEW_HPP
#define RDF4CPP_IRIVIEW_HPP

#include <rdf4cpp/InvalidNode.hpp>

#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace rdf4cpp {

/**
 * std::format gets you a printable string.
 */
enum struct IRIParseError : uint8_t {
    UnknownPrefix,
    UnexpectedRelative,
    InvalidScheme,
    InvalidUserinfo,
    InvalidHost,
    InvalidPort,
    InvalidPath,
    InvalidQuery,
    InvalidFragment,
    InvalidPrefix,
};

[[nodiscard]] std::string_view to_string_view(IRIParseError ipe) noexcept;

struct InvalidIRI : InvalidNode {
    using error_type = IRIParseError;

private:
    error_type type_;

public:
    explicit InvalidIRI(error_type ty)
        : InvalidNode{std::format("Invalid IRI: {}", to_string_view(ty))},
          type_{ty} {
    }

    [[nodiscard]] error_type type() const noexcept {
        return type_;
    }
};

/**
 * wrapper around a std::string_view that allows to access it as its IRI components.
 */
struct IRIView {
private:
    std::string_view data;

public:
    explicit IRIView(std::string_view iri) noexcept;

private:
    /**
     * note that even if defined = false, start and len are valid.
     * they refer to a 0 length location where the relevant part could have been and where the next part should be searched.
     */
    struct IRIPart {
        size_t start, len;
        bool defined;
    };

    /**
     * ->scheme
     * @return
     */
    [[nodiscard]] IRIPart get_scheme_part() const noexcept;
    /**
     * scheme->authority
     * @param scheme
     * @return
     */
    [[nodiscard]] IRIPart get_authority_part(const IRIPart &scheme) const noexcept;
    /**
     * authority->path
     * @param auth
     * @return
     */
    [[nodiscard]] IRIPart get_path_part(const IRIPart &auth) const noexcept;
    /**
     * path->query
     * @param path
     * @return
     */
    [[nodiscard]] IRIPart get_query_part(const IRIPart &path) const noexcept;
    /**
     * query->fragment
     * @param query
     * @return
     */
    [[nodiscard]] IRIPart get_fragment_part(const IRIPart &query) const noexcept;

    /**
     * authority->userinfo
     * @param auth
     * @return
     */
    [[nodiscard]] IRIPart get_userinfo_part(const IRIPart& auth) const noexcept;
    /**
     * authority,userinfo->host
     * @param auth
     * @param uinfo
     * @return
     */
    [[nodiscard]] IRIPart get_host_part(const IRIPart& auth, const IRIPart& uinfo) const noexcept;
    /**
     * authority,host->port
     * @param auth
     * @param host
     * @return
     */
    [[nodiscard]] IRIPart get_port_part(const IRIPart& auth, const IRIPart& host) const noexcept;

    /**
     * gets the string_view representing the part (empty on undefined)
     * @param r
     * @return
     */
    [[nodiscard]] std::string_view apply(const IRIPart &r) const noexcept;
    /**
     * gets the string_view representing the part or std::nullopt if undefined.
     * @param r
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> apply_opt(const IRIPart &r) const noexcept;

public:
    /**
     * is this IRI a relative reference?
     * equivalent to scheme().empty()
     * @return
     */
    [[nodiscard]] bool is_relative() const noexcept;
    /**
     * the scheme part of the IRI
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> scheme() const noexcept;
    /**
     * the authority part of the IRI
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> authority() const noexcept;
    /**
     * the path of the IRI
     * @return
     */
    [[nodiscard]] std::string_view path() const noexcept;
    /**
     * the query part of the IRI
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> query() const noexcept;
    /**
     * the fragment part of the IRI
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> fragment() const noexcept;

    struct AllParts {
        std::optional<std::string_view> scheme;
        std::optional<std::string_view> authority;
        std::string_view path;
        std::optional<std::string_view> query;
        std::optional<std::string_view> fragment;
    };
    /**
     * all parts of the IRI. intended to be used with structured bindings:
     * auto [scheme, auth, path, query, frag] = iri_view.all_parts();
     * more efficient than querying each part individually via its method.
     * @return
     */
    [[nodiscard]] AllParts all_parts() const noexcept;

    /**
     * everything except the fragment part of the IRI.
     * @return
     */
    [[nodiscard]] std::string_view to_absolute() const noexcept;

    /**
     * the userinfo part of the authority.
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> userinfo() const noexcept;
    /**
     * the host part of the authority.
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> host() const noexcept;
    /**
     * the port part of the authority.
     * @return
     */
    [[nodiscard]] std::optional<std::string_view> port() const noexcept;

    struct AuthorityParts {
        std::optional<std::string_view> userinfo, host, port;
    };
    /**
     * all authority parts of the IRI. intended to be used with structured bindings:
     * auto [userinfo, host, port] = iri_view.all_authority_parts();
     * more efficient than querying each part individually via its method.
     * @return
     */
    [[nodiscard]] AuthorityParts all_authority_parts() const noexcept;

    /**
     * quickly checks if the IRI is valid according to the IRI specification (not the schemes specification).
     * (does not accept relative references)
     * (may miss invalid IRIs)
     * @throws InvalidIRI if validation failed
     */
    void quick_validate(bool allow_relative = false) const;
};

} // namespace rdf4cpp

template<>
struct std::formatter<rdf4cpp::IRIParseError> : std::formatter<string_view> {
    template<typename Ctx>
    auto format(rdf4cpp::IRIParseError const p, Ctx &ctx) const {
        return std::formatter<string_view>::format(to_string_view(p), ctx);
    }
};

#endif  //RDF4CPP_IRIVIEW_HPP
