#ifndef RDF4CPP_IRIVIEW_HPP
#define RDF4CPP_IRIVIEW_HPP

#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace rdf4cpp {
/**
 * std::format gets you a printable string.
 */
enum struct IRIFactoryError {
    Ok,
    UnknownPrefix,
    Relative,
    InvalidScheme,
    InvalidUserinfo,
    InvalidHost,
    InvalidPort,
    InvalidPath,
    InvalidQuery,
    InvalidFragment,
    InvalidPrefix,
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
     * @return
     */
    [[nodiscard]] IRIFactoryError quick_validate(bool allow_relative = false) const noexcept;
};
}

template<>
struct std::formatter<rdf4cpp::IRIFactoryError> : std::formatter<string_view> {
    auto format(rdf4cpp::IRIFactoryError p, format_context& ctx) const {
        std::string_view s{};
        switch (p) {
            case rdf4cpp::IRIFactoryError::Ok:
                s = "Ok";
                break;
            case rdf4cpp::IRIFactoryError::UnknownPrefix:
                s = "UnknownPrefix";
                break;
            case rdf4cpp::IRIFactoryError::Relative:
                s = "Relative";
                break;
            case rdf4cpp::IRIFactoryError::InvalidScheme:
                s = "InvalidScheme";
                break;
            case rdf4cpp::IRIFactoryError::InvalidUserinfo:
                s = "InvalidUserinfo";
                break;
            case rdf4cpp::IRIFactoryError::InvalidHost:
                s = "InvalidHost";
                break;
            case rdf4cpp::IRIFactoryError::InvalidPort:
                s = "InvalidPort";
                break;
            case rdf4cpp::IRIFactoryError::InvalidPath:
                s = "InvalidPath";
                break;
            case rdf4cpp::IRIFactoryError::InvalidQuery:
                s = "InvalidQuery";
                break;
            case rdf4cpp::IRIFactoryError::InvalidFragment:
                s = "InvalidFragment";
                break;
            default:
                s = "Unknown";
                break;
        }
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

#endif  //RDF4CPP_IRIVIEW_HPP
