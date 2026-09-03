#include "CompensatedSum.hpp"

#include <rdf4cpp/datatypes/registry/DatatypeRegistry.hpp>
#include <rdf4cpp/datatypes/xsd/integers/signed/Integer.hpp>

namespace rdf4cpp {

CompensatedSum::CompensatedSum(storage::DynNodeStoragePtr node_storage)
    : node_storage_{node_storage} {
}

bool CompensatedSum::is_exact(IRI const &datatype) {
    if (datatype.null()) {
        return false;  // the null-value, let the deferred ops propagate it
    }

    if (datatype == cached_datatype_) {
        return cached_exact_;
    }

    auto const *entry = datatypes::registry::DatatypeRegistry::get_entry(datatypes::registry::DatatypeIDView{datatype});

    cached_datatype_ = datatype;
    cached_exact_ = entry != nullptr && entry->numeric_ops.has_value() && entry->numeric_ops->is_exact;
    return cached_exact_;
}

void CompensatedSum::add(Literal const &lit) {
    this->add(make_deferred_from_literal(lit));
}

void CompensatedSum::add(DeferredValue const &value) {
    if (!sum_.has_value()) {
        // the first value seeds the sum; adding it to a "0"^^xsd:integer instead would poison
        // owl:rational and owl:real, which have no common numeric type with xsd:integer
        sum_ = value;
        comp_ = numeric_sub_deferred(value, value, node_storage_);  // zero, in the datatype of value
        compensating_ = !this->is_exact(value.second);
        return;
    }

    if (!compensating_ && this->is_exact(value.second)) {
        // arbitrary precision, so there is no rounding error to carry
        sum_ = numeric_add_deferred(*sum_, value, node_storage_);
        return;
    }

    compensating_ = true;

    // Kahan: add back what the previous steps lost, then record what this step loses
    auto const y = numeric_sub_deferred(value, comp_, node_storage_);
    auto const t = numeric_add_deferred(*sum_, y, node_storage_);
    comp_ = numeric_sub_deferred(numeric_sub_deferred(t, *sum_, node_storage_), y, node_storage_);
    sum_ = t;
}

Literal CompensatedSum::value() const {
    if (!sum_.has_value()) {
        return Literal::make_typed_from_value<datatypes::xsd::Integer>(0);
    }

    if (!compensating_) {
        return materialize_deferred(*sum_, node_storage_);
    }

    return materialize_deferred(numeric_add_deferred(*sum_, comp_, node_storage_), node_storage_);
}

}  // namespace rdf4cpp
