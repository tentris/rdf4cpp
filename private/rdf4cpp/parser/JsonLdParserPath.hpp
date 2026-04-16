#ifndef RDF4CPP_JSONLDPARSERPATH_HPP
#define RDF4CPP_JSONLDPARSERPATH_HPP

#include <string>
#include <variant>
#include <vector>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace json_ld {
        struct KeyPath {
            std::vector<std::variant<std::string, size_t>> keys;
        };
    }  // namespace json_ld

    template<typename T>
    // ReSharper disable once CppDFAUnreachableFunctionCall
    static auto try_get_field(simdjson::ondemand::object o, std::string_view key) {
        T r{};
        auto c = o[key].get(r);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        return std::tuple(c, r);
    }
    template<typename T>
    // ReSharper disable once CppDFAUnreachableFunctionCall
    static auto try_get_field(simdjson::ondemand::array a, size_t key) {
        T r{};
        a.reset();
        auto c = a.at(key).get(r);
        return std::tuple(c, r);
    }
    template<typename T>
    static auto try_get_field(simdjson::ondemand::object o, json_ld::KeyPath const &key) {
        if (key.keys.empty()) {
            if constexpr (std::same_as<T, simdjson::ondemand::object>) {
                return std::tuple(simdjson::SUCCESS, o);
            } else {
                return std::tuple(simdjson::INCORRECT_TYPE, T{});
            }
        }
        simdjson::error_code c;
        std::variant<simdjson::ondemand::array, simdjson::ondemand::object> e = o;
        for (size_t i = 0; i < key.keys.size() - 1; ++i) {
            auto const &next_key = key.keys[i + 1];  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            auto const &current_key = key.keys[i];   // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            if (std::holds_alternative<size_t>(next_key)) {
                if (std::holds_alternative<size_t>(current_key)) {
                    std::tie(c, e) = try_get_field<simdjson::ondemand::array>(std::get<simdjson::ondemand::array>(e), std::get<size_t>(current_key));
                } else {
                    std::tie(c, e) = try_get_field<simdjson::ondemand::array>(std::get<simdjson::ondemand::object>(e), std::get<std::string>(current_key));
                }
            } else {
                if (std::holds_alternative<size_t>(current_key)) {
                    std::tie(c, e) = try_get_field<simdjson::ondemand::object>(std::get<simdjson::ondemand::array>(e), std::get<size_t>(current_key));
                } else {
                    std::tie(c, e) = try_get_field<simdjson::ondemand::object>(std::get<simdjson::ondemand::object>(e), std::get<std::string>(current_key));
                }
            }
            if (c != simdjson::SUCCESS) {
                return std::tuple(c, T{});
            }
        }
        auto const &current_key = key.keys[key.keys.size() - 1];  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        if (std::holds_alternative<size_t>(current_key)) {
            return try_get_field<T>(std::get<simdjson::ondemand::array>(e), std::get<size_t>(current_key));
        } else {
            return try_get_field<T>(std::get<simdjson::ondemand::object>(e), std::get<std::string>(current_key));
        }
    }
    template<typename T>
    static std::tuple<simdjson::error_code, std::optional<T>> try_get_optional_field(simdjson::ondemand::object o,
                                                                                     std::string_view key) {
        simdjson::ondemand::value v;
        auto c = o[key].get(v);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        if (c != simdjson::SUCCESS) {
            return std::tuple(c, std::nullopt);
        }
        if (v.is_null()) {
            return std::tuple(c, std::nullopt);
        }
        T r{};
        c = v.get(r);
        return std::tuple(c, r);
    }
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDPARSERPATH_HPP
