#ifndef RDF4CPP_REFERENCENODESTORAGE_COMMON_HPP
#define RDF4CPP_REFERENCENODESTORAGE_COMMON_HPP

#include <tuple>
#include <utility>
#include <dice/template-library/tuple_algorithm.hpp>
#include <rdf4cpp/datatypes/xsd.hpp>
#include <rdf4cpp/storage/identifier/LiteralType.hpp>

namespace rdf4cpp::storage::reference_node_storage::specialization_detail {

/**
 * Generates a storage specialization lookup-table for the reference node storages.
 * The returned array is indexed with identifier::LiteralType and contains true
 * if the node storage has a specialized storage for the type and false if it does not.
 *
 * @tparam Tuple a tuple of (Sync|Unsync)NodeTypeStorage<SpecializedLiteralBackend<...>> containing all the specialized backends
 *          for the node storage. In our case this is `decltype( (Sync|Unsync)ReferenceNodeStorageBackend::specialized_literal_storage_ )`
 * @return array that contains true in every slot for which there is a specialized backend
 */
template<typename Tuple>
static consteval std::array<bool, 1 << identifier::LiteralType::width> make_storage_specialization_lut() noexcept {
    std::array<bool, 1 << identifier::LiteralType::width> ret{};

    dice::template_library::tuple_type_for_each<Tuple>([&]<typename T>() {
        ret[T::backend_type::literal_type::fixed_id.to_underlying()] = true;
    });

    return ret;
}

/**
 * Calls the given function f with the specialized object for the given datatype
 *
 * @param container any container for specialized things in the correct order (i.e. specialized_literal_storage_ or next_specialized_literal_ids_)
 * @param datatype the datatype of the specialized object
 * @param f the function to call with the corresponding specialized object
 * @return whatever f returns
 */
template<typename S, typename F>
decltype(auto) visit_specialized(S &&container, identifier::LiteralType const datatype, F f) {
    using namespace rdf4cpp::datatypes;

    // manually translate runtime knowledge to compiletime
    // can probably be done using metaprogramming, but this is faster
    switch (datatype.to_underlying()) {
        case xsd::Integer::fixed_id.to_underlying():
            return f(std::get<0>(std::forward<S>(container)));
        case xsd::NonNegativeInteger::fixed_id.to_underlying():
            return f(std::get<1>(std::forward<S>(container)));
        case xsd::PositiveInteger::fixed_id.to_underlying():
            return f(std::get<2>(std::forward<S>(container)));
        case xsd::NonPositiveInteger::fixed_id.to_underlying():
            return f(std::get<3>(std::forward<S>(container)));
        case xsd::NegativeInteger::fixed_id.to_underlying():
            return f(std::get<4>(std::forward<S>(container)));
        case xsd::Long::fixed_id.to_underlying():
            return f(std::get<5>(std::forward<S>(container)));
        case xsd::UnsignedLong::fixed_id.to_underlying():
            return f(std::get<6>(std::forward<S>(container)));
        case xsd::Decimal::fixed_id.to_underlying():
            return f(std::get<7>(std::forward<S>(container)));
        case xsd::Double::fixed_id.to_underlying():
            return f(std::get<8>(std::forward<S>(container)));
        case xsd::Base64Binary::fixed_id.to_underlying():
            return f(std::get<9>(std::forward<S>(container)));
        case xsd::HexBinary::fixed_id.to_underlying():
            return f(std::get<10>(std::forward<S>(container)));
        case xsd::Date::fixed_id.to_underlying():
            return f(std::get<11>(std::forward<S>(container)));
        case xsd::DateTime::fixed_id.to_underlying():
            return f(std::get<12>(std::forward<S>(container)));
        case xsd::DateTimeStamp::fixed_id.to_underlying():
            return f(std::get<13>(std::forward<S>(container)));
        case xsd::GYearMonth::fixed_id.to_underlying():
            return f(std::get<14>(std::forward<S>(container)));
        case xsd::Duration::fixed_id.to_underlying():
            return f(std::get<15>(std::forward<S>(container)));
        case xsd::DayTimeDuration::fixed_id.to_underlying():
            return f(std::get<16>(std::forward<S>(container)));
        case xsd::YearMonthDuration::fixed_id.to_underlying():
            return f(std::get<17>(std::forward<S>(container)));
        default:
            RDF4CPP_UNREACHABLE;
    }
}

} // rdf4cpp::storage::reference_node_storage::specialization_detail

#endif  //RDF4CPP_REFERENCENODESTORAGE_COMMON_HPP
