#include "CompensatedSum.hpp"

#include <rdf4cpp/datatypes/registry/DatatypeRegistry.hpp>
#include <rdf4cpp/datatypes/xsd/integers/signed/Integer.hpp>

#include "Literal.hpp"

namespace rdf4cpp {

CompensatedSum::CompensatedSum(storage::DynNodeStoragePtr node_storage)
    : node_storage_{node_storage} {
}

bool CompensatedSum::is_exact(IRI const &datatype) {
    if (datatype.null()) {
        return false; // the null-value, let the deferred ops propagate it
    }

    if (datatype == cached_datatype_) {
        return cached_exact_;
    }

    auto const *entry = datatypes::registry::DatatypeRegistry::get_entry(datatypes::registry::DatatypeIDView{datatype});

    cached_datatype_ = datatype;
    cached_exact_ = entry != nullptr && entry->numeric_ops.has_value() && entry->numeric_ops->is_exact;
    return cached_exact_;
}

void CompensatedSum::add(Literal const &lit, uint64_t multiplicity) {
    this->add(make_deferred_from_literal(lit), multiplicity);
}

void CompensatedSum::add(DeferredLiteral const &value, uint64_t multiplicity) {
    // assumption: multiplicity is usually small
    while (multiplicity > 0) {
        add_once(value);
        multiplicity -= 1;
    }
}

void CompensatedSum::add_once(DeferredLiteral const &value) {
    if (!sum_.has_value()) {
        // the first value seeds the sum; adding it to a "0"^^xsd:integer instead would poison
        // owl:rational and owl:real, which have no common numeric type with xsd:integer
        if (!value.datatype.is_numeric_datatype()) {
            // poison
            sum_.emplace();
            return;
        }

        sum_ = value;
        compensating_ = !is_exact(value.datatype);
        return;
    }

    if (!compensating_ && is_exact(value.datatype)) {
        // arbitrary precision, so there is no rounding error to carry
        sum_ = numeric_add_deferred(*sum_, value, node_storage_);
        return;
    }

    compensating_ = true;

    // Kahan-Babuska-Neumaier: collect the loss of each addition on the side and add it back in value().
    auto [new_sum, loss] = numeric_add_with_loss_deferred(*sum_, value, node_storage_);

    // a null loss is one there is nothing to account for: the total went infinite, or it is
    // poisoned and sum_ carries that on. comp_ itself is the null-value until the first loss seeds it
    if (!loss.null()) {
        comp_ = comp_.null() ? loss : numeric_add_deferred(comp_, loss, node_storage_);
    }

    sum_ = std::move(new_sum);
}

Literal CompensatedSum::value() const {
    Literal final_result{};
    if (!sum_.has_value()) {
        final_result = nullary_sum(node_storage_);
    } else if (comp_.null()) {
        final_result = materialize_deferred(*sum_, node_storage_);  // nothing was lost (yet)
    } else {
        final_result = materialize_deferred(numeric_add_deferred(*sum_, comp_, node_storage_), node_storage_);
    }

    if (!final_result.is_numeric()) {
        final_result = Literal{};
    }

    return final_result;
}

bool CompensatedSum::poisoned() const noexcept {
    return sum_.has_value() && sum_->null();
}

Literal CompensatedSum::nullary_sum(storage::DynNodeStoragePtr node_storage) {
    return Literal::make_typed_from_value<datatypes::xsd::Integer>(0, node_storage);
}

}  // namespace rdf4cpp
