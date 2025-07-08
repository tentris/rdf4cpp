#ifndef RDF4CPP_DATATYPECONVERSIONTYPING_HPP
#define RDF4CPP_DATATYPECONVERSIONTYPING_HPP

#include <any>
#include <concepts>
#include <tuple>
#include <type_traits>

#include <rdf4cpp/datatypes/LiteralDatatype.hpp>
#include <rdf4cpp/datatypes/registry/DatatypeID.hpp>
#include <rdf4cpp/datatypes/registry/util/TypeList.hpp>

namespace rdf4cpp::datatypes::registry {

/**
 * A conversion from a source_type to a target_type
 */
template<typename T>
concept ConversionEntry = requires(typename T::source_type::cpp_type value) {
                              requires LiteralDatatype<typename T::source_type>;
                              requires LiteralDatatype<typename T::target_type>;
                              { T::convert(value) } -> std::same_as<nonstd::expected<typename T::target_type::cpp_type, DynamicError>>;
                          };


namespace conversion_typing_detail {

template<typename T>
struct IsConversionLayer : std::false_type {};

template<typename... Ts>
struct IsConversionLayer<mz::type_list<Ts...>>
    : std::bool_constant<
              (ConversionEntry<Ts> && ...) && util::all_types_same<typename Ts::source_type...>::value> {};


template<typename Layer>
struct ConversionLayerSource;

template<typename Entry, typename... Entries>
struct ConversionLayerSource<mz::type_list<Entry, Entries...>> {
    using type = typename Entry::source_type;
};

template<typename T>
struct IsConversionTable
    : std::false_type {};

template<typename... Ts>
struct IsConversionTable<mz::type_list<Ts...>>
    : std::bool_constant<
              (IsConversionLayer<Ts>::value && ...) && util::all_types_same<typename ConversionLayerSource<Ts>::type...>::value> {};

}  // namespace conversion_typing_detail


/**
 * A tuple of ConversionEntries
 * where all elements have the same associated source_type defined
 */
template<typename T>
concept ConversionLayer = conversion_typing_detail::IsConversionLayer<T>::value;

/**
 * A tuple of ConversionLayers
 * where all all ConversionEntries in all ConversionLayers have the same associated source_type defined.
 */
template<typename T>
concept ConversionTable = conversion_typing_detail::IsConversionTable<T>::value;

/**
 * A type erased version of a ConversionEntry.
 */
struct RuntimeConversionEntry {
    using convert_fptr_t = nonstd::expected<std::any, DynamicError> (*)(std::any const &) noexcept;
    using inverted_convert_fptr_t = nonstd::expected<std::any, DynamicError> (*)(std::any const &) noexcept;

    DatatypeID target_type_id;
    convert_fptr_t convert;
    inverted_convert_fptr_t inverted_convert;

    template<ConversionEntry Entry>
    static RuntimeConversionEntry from_concrete() noexcept {
        DatatypeID target_type_iri = []() noexcept {
            if constexpr (FixedIdLiteralDatatype<typename Entry::target_type>) {
                return DatatypeID{Entry::target_type::fixed_id};
            } else {
                return DatatypeID{std::string{Entry::target_type::identifier}};
            }
        }();

        return RuntimeConversionEntry{
                .target_type_id = std::move(target_type_iri),
                .convert = [](std::any const &value) noexcept -> nonstd::expected<std::any, DynamicError> {
                    auto const actual_value = std::any_cast<typename Entry::source_type::cpp_type>(value);
                    return std::any{Entry::convert(actual_value)};
                },
                .inverted_convert = [](std::any const &value) noexcept -> nonstd::expected<std::any, DynamicError> {
                    auto const actual_value = std::any_cast<typename Entry::target_type::cpp_type>(value);
                    auto const maybe_converted = Entry::inverse_convert(actual_value);

                    if (!maybe_converted.has_value()) {
                        return nonstd::make_unexpected(maybe_converted.error());
                    }

                    return std::any{*maybe_converted};
                }};
    }
};

/**
 * A runtime accessible version of a compile-time conversion table.
 */
struct RuntimeConversionTable {
private:
    size_t s_rank;
    size_t max_p_rank;
    std::vector<size_t> p_ranks;
    std::vector<RuntimeConversionEntry> table;

    RuntimeConversionTable(size_t const s_rank, size_t const max_p_rank) noexcept
        : s_rank{s_rank}, max_p_rank{max_p_rank}, p_ranks(s_rank)
    {
        if (s_rank * max_p_rank > 0) {
            table.resize(s_rank * max_p_rank, RuntimeConversionEntry{
                                                      .target_type_id = DatatypeID{storage::identifier::LiteralType{}},
                                                      .convert = nullptr,
                                                      .inverted_convert = nullptr});
        }
    }

    inline static RuntimeConversionTable empty() noexcept {
        return RuntimeConversionTable{0, 0};
    }

    /**
     * mutable version of conversion_at_index
     * for more details see: const version of conversion_at_index
     */
    inline RuntimeConversionEntry &conversion_at_index(size_t s_off, size_t p_off) noexcept {
        assert(s_off < s_rank);
        assert(p_off < promotion_rank_at_level(s_off));

        return table[s_off * max_p_rank + p_off];
    }


    friend struct DatatypeRegistry;

public:
    template<ConversionTable Table>
    static RuntimeConversionTable from_concrete() noexcept {
        static constexpr size_t s_rank = Table::length;

        static constexpr size_t max_p_rank = util::type_list_fold<Table>(0ul, []<ConversionLayer Layer>(auto acc) {
            return std::max(acc, Layer::length);
        });

        RuntimeConversionTable table{s_rank, max_p_rank};

        util::type_list_for_each_with<Table>(0ul, [&]<ConversionLayer Layer>(size_t const s_off) noexcept {
            table.p_ranks[s_off] = Layer::length;

            util::type_list_for_each_with<Layer>(0ul, [&]<ConversionEntry Entry>(size_t const p_off) noexcept {
                table.conversion_at_index(s_off, p_off) = RuntimeConversionEntry::from_concrete<Entry>();
                return p_off + 1;
            });

            return s_off + 1;
        });

        return table;
    }

    /**
     * @return the type's subtype rank
     */
    [[nodiscard]] inline size_t subtype_rank() const noexcept {
        return s_rank;
    }

    /**
     * @param subtype_offset how many subtype substitutions up to calculate the promotion rank
     * @return the type's promotion rank at a given subtype offset
     */
    [[nodiscard]] inline size_t promotion_rank_at_level(size_t const subtype_offset) const noexcept {
        return p_ranks[subtype_offset];
    }

    /**
     * Get the defined conversion for a given subtype offset and promotion offset.
     *
     * SAFETY: the user needs to ensure the following assertions hold, otherwise the behaviour is undefined
     *      - s_off < s_rank
     *      - p_off < promotion rank at subtype offset s_off
     *
     * @param s_off subtype offset, how many subtype substitutions up the conversion is
     * @param p_off promotion offset, how many promotions right given the subtype offset the conversion is
     * @return the found conversion
     */
    [[nodiscard]] inline RuntimeConversionEntry const &conversion_at_index(size_t s_off, size_t p_off) const noexcept {
        assert(s_off < s_rank);
        assert(p_off < promotion_rank_at_level(s_off));

        return table[s_off * max_p_rank + p_off];
    }
};

}  // namespace rdf4cpp::datatypes::registry

#endif  //RDF4CPP_DATATYPECONVERSIONTYPING_HPP
