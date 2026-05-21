#ifndef RDF4CPP_JSONLDPARSERPATH_HPP
#define RDF4CPP_JSONLDPARSERPATH_HPP

#include <string>
#include <variant>
#include <vector>

#include <simdjson.h>

namespace rdf4cpp::parser {
    namespace json_ld {
        /**
         * represents a path of object/map keys and array indices that lead from an object/map to a value.
         * for API reasons always starts at an object, so the first entry is expected to be a key (std::string).
         */
        struct KeyPath {
            std::vector<std::variant<std::string, size_t>> keys;
        };
    }  // namespace json_ld

    template<typename T>
    // ReSharper disable once CppDFAUnreachableFunctionCall
    static auto try_get_field(simdjson::ondemand::object obj, std::string_view key) {
        T result{};
        auto error_code = obj[key].get(result);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        return std::tuple(error_code, result);
    }
    template<typename T>
    // ReSharper disable once CppDFAUnreachableFunctionCall
    static auto try_get_field(simdjson::ondemand::array array, size_t key) {
        T result{};
        array.reset();
        auto error_code = array.at(key).get(result);
        return std::tuple(error_code, result);
    }
    template<typename T>
    static auto try_get_field(simdjson::ondemand::object object, json_ld::KeyPath const &key) {
        if (key.keys.empty()) {
            if constexpr (std::same_as<T, simdjson::ondemand::object>) {
                return std::tuple(simdjson::SUCCESS, object);
            } else {
                return std::tuple(simdjson::INCORRECT_TYPE, T{});
            }
        }
        simdjson::error_code error_code;
        std::variant<simdjson::ondemand::array, simdjson::ondemand::object> current = object;
        for (size_t i = 0; i < key.keys.size() - 1; ++i) {
            auto const &next_key = key.keys[i + 1];  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            auto const &current_key = key.keys[i];   // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            // current_key determines if we expect current to be an array or object.
            // since we follow a path that has already been checked, we expect this to match (std::get throws if not)
            //
            // next_key determines if we need an array or object to index in the next step
            // for the same reasons as above, we expect this to exist
            // but just in case, it will return an error code, if it does not
            if (std::holds_alternative<size_t>(next_key)) {
                if (std::holds_alternative<size_t>(current_key)) {
                    std::tie(error_code, current) = try_get_field<simdjson::ondemand::array>(std::get<simdjson::ondemand::array>(current), std::get<size_t>(current_key));
                } else {
                    std::tie(error_code, current) = try_get_field<simdjson::ondemand::array>(std::get<simdjson::ondemand::object>(current), std::get<std::string>(current_key));
                }
            } else {
                if (std::holds_alternative<size_t>(current_key)) {
                    std::tie(error_code, current) = try_get_field<simdjson::ondemand::object>(std::get<simdjson::ondemand::array>(current), std::get<size_t>(current_key));
                } else {
                    std::tie(error_code, current) = try_get_field<simdjson::ondemand::object>(std::get<simdjson::ondemand::object>(current), std::get<std::string>(current_key));
                }
            }
            if (error_code != simdjson::SUCCESS) {
                return std::tuple(error_code, T{});
            }
        }
        auto const &current_key = key.keys[key.keys.size() - 1];  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        if (std::holds_alternative<size_t>(current_key)) {
            return try_get_field<T>(std::get<simdjson::ondemand::array>(current), std::get<size_t>(current_key));
        } else {
            return try_get_field<T>(std::get<simdjson::ondemand::object>(current), std::get<std::string>(current_key));
        }
    }
    template<typename T>
    static std::tuple<simdjson::error_code, std::optional<T>> try_get_optional_field(simdjson::ondemand::object obj,
                                                                                     std::string_view key) {
        simdjson::ondemand::value val;
        auto error_code = obj[key].get(val);  // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
        if (error_code != simdjson::SUCCESS) {
            return std::tuple(error_code, std::nullopt);
        }
        if (val.is_null()) {
            return std::tuple(error_code, std::nullopt);
        }
        T result{};
        error_code = val.get(result);
        return std::tuple(error_code, result);
    }
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_JSONLDPARSERPATH_HPP
