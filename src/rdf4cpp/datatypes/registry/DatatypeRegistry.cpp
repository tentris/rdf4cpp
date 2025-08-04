#include "DatatypeRegistry.hpp"

namespace rdf4cpp::datatypes::registry {

bool relaxed_parsing_mode = false;

DatatypeRegistry::registered_datatypes_t &DatatypeRegistry::get_mutable() noexcept {
    static registered_datatypes_t registry_ = []() {
        registered_datatypes_t r;
        r.resize(dynamic_datatype_offset, DatatypeEntry::placeholder()); // placeholders for fixed id datatypes

        return r;
    }();

    return registry_;
}

void DatatypeRegistry::add_fixed(DatatypeEntry entry_to_add, LiteralType type_id) noexcept {
    auto const id_as_index = static_cast<size_t>(type_id.to_underlying()) - 1; // ids from 1 to n stored in places 0 to n-1
    RDF4CPP_ASSERT(id_as_index < dynamic_datatype_offset);

    auto &slot = DatatypeRegistry::get_mutable()[id_as_index];
    RDF4CPP_ASSERT(slot.datatype_iri.empty()); // is placeholder
    slot = std::move(entry_to_add);
}

void DatatypeRegistry::add(DatatypeEntry entry_to_add) noexcept {
    auto &registry = DatatypeRegistry::get_mutable();

    auto found = std::find_if(registry.begin() + dynamic_datatype_offset, registry.end(), [&](const auto &entry) { return entry.datatype_iri == entry_to_add.datatype_iri; });
    if (found == registry.end()) {
        registry.push_back(std::move(entry_to_add));

        std::sort(registry.begin() + dynamic_datatype_offset, registry.end(),
                  [](const auto &left, const auto &right) { return left.datatype_iri < right.datatype_iri; });
    } else {
        *found = std::move(entry_to_add);
    }
}

DatatypeRegistry::registered_datatypes_t const &DatatypeRegistry::registered_datatypes() noexcept {
    return DatatypeRegistry::get_mutable();
}

std::optional<std::string_view> DatatypeRegistry::get_iri(DatatypeIDView const datatype_id) noexcept {
    return find_map_entry(datatype_id, [](auto const &entry) noexcept -> std::string_view {
        return entry.datatype_iri;
    });
}

DatatypeRegistry::DatatypeEntry const *DatatypeRegistry::get_entry(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return &entry;
    });

    return res.has_value() ? *res : nullptr;
}

DatatypeRegistry::factory_fptr_t DatatypeRegistry::get_factory(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return entry.factory_fptr;
    });

    return res.has_value() ? *res : nullptr;
}

DatatypeRegistry::serialize_fptr_t DatatypeRegistry::get_serialize_canonical_string(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return entry.serialize_canonical_string_fptr;
    });

    return res.has_value() ? *res : nullptr;
}

DatatypeRegistry::serialize_fptr_t DatatypeRegistry::get_serialize_simplified_string(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return entry.serialize_simplified_string_fptr;
    });

    return res.has_value() ? *res : nullptr;
}


DatatypeRegistry::NumericOps const *DatatypeRegistry::get_numerical_ops(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return &entry.numeric_ops;
    });

    // res is nullopt if no datatype matching given datatype_iri was found
    if (res.has_value()) {
        // contained ptr cannot be nullptr as by return in lambda for find_map_entry above
        // optional behind contained ptr can be nullopt if type is not numeric
        if (auto const ops_ptr = res.value(); ops_ptr->has_value()) {
            return &ops_ptr->value();
        }
    }

    // no datatype found or not numeric
    return nullptr;
}

DatatypeRegistry::TimepointOps const *DatatypeRegistry::get_timepoint_ops(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return &entry.timepoint_ops;
    });

    // res is nullopt if no datatype matching given datatype_iri was found
    if (res.has_value()) {
        // contained ptr cannot be nullptr as by return in lambda for find_map_entry above
        // optional behind contained ptr can be nullopt if type is not numeric
        if (auto const ops_ptr = res.value(); ops_ptr->has_value()) {
            return &ops_ptr->value();
        }
    }

    // no datatype found or not numeric
    return nullptr;
}

DatatypeRegistry::DurationOps const *DatatypeRegistry::get_duration_ops(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return &entry.duration_ops;
    });

    // res is nullopt if no datatype matching given datatype_iri was found
    if (res.has_value()) {
        // contained ptr cannot be nullptr as by return in lambda for find_map_entry above
        // optional behind contained ptr can be nullopt if type is not numeric
        if (auto const ops_ptr = res.value(); ops_ptr->has_value()) {
            return &ops_ptr->value();
        }
    }

    // no datatype found or not numeric
    return nullptr;
}

DatatypeRegistry::ebv_fptr_t DatatypeRegistry::get_ebv(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return entry.ebv_fptr;
    });

    return res.has_value() ? *res : nullptr;
}

DatatypeRegistry::compare_fptr_t DatatypeRegistry::get_compare(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return entry.compare_fptr;
    });

    return res.has_value() ? *res : nullptr;
}

DatatypeRegistry::InliningOps const *DatatypeRegistry::get_inlining_ops(DatatypeIDView const datatype_id) noexcept {
    auto const res = find_map_entry(datatype_id, [](auto const &entry) noexcept {
        return &entry.inlining_ops;
    });

    // res is nullopt if no datatype matching given datatype_iri was found
    if (res.has_value()) {
        // contained ptr cannot be nullptr as by return in lambda for find_map_entry above
        // optional behind contained ptr can be nullopt if type is not inlineable
        if (auto const ops_ptr = res.value(); ops_ptr->has_value()) {
            return &ops_ptr->value();
        }
    }

    // no datatype found or not inlineable
    return nullptr;
}

std::optional<DatatypeRegistry::DatatypeConverter> DatatypeRegistry::get_common_numeric_op_type_conversion(DatatypeEntry const &lhs_entry, DatatypeEntry const &rhs_entry) noexcept {
    RDF4CPP_ASSERT(lhs_entry.numeric_ops.has_value());
    RDF4CPP_ASSERT(rhs_entry.numeric_ops.has_value());

    size_t const lhs_init_soff = lhs_entry.numeric_ops->is_stub() ? lhs_entry.numeric_ops->get_stub().start_s_off : 0;
    size_t const rhs_init_soff = rhs_entry.numeric_ops->is_stub() ? rhs_entry.numeric_ops->get_stub().start_s_off : 0;

    return get_common_type_conversion(lhs_entry.conversion_table, rhs_entry.conversion_table, lhs_init_soff, rhs_init_soff);
}

RuntimeConversionEntry const &DatatypeRegistry::get_numeric_op_impl_conversion(DatatypeEntry const &entry) noexcept {
    RDF4CPP_ASSERT(entry.numeric_ops.has_value());
    RDF4CPP_ASSERT(entry.numeric_ops->is_stub());

    return entry.conversion_table.conversion_at_index(entry.numeric_ops->get_stub().start_s_off, 0);
}


std::optional<DatatypeRegistry::DatatypeConverter> DatatypeRegistry::get_common_type_conversion(DatatypeIDView const lhs_type_id, DatatypeIDView const rhs_type_id) noexcept {
    auto const lhs_entry = get_entry(lhs_type_id);
    if (lhs_entry == nullptr) {
        return std::nullopt;
    }

    auto const &rhs_entry = get_entry(rhs_type_id);
    if (rhs_entry == nullptr) {
        return std::nullopt;
    }

    return get_common_type_conversion(lhs_entry->conversion_table, rhs_entry->conversion_table);
}

std::optional<DatatypeRegistry::DatatypeConverter> DatatypeRegistry::get_common_type_conversion(
        RuntimeConversionTable const &lhs_conv,
        RuntimeConversionTable const &rhs_conv,
        size_t const lhs_init_soff,
        size_t const rhs_init_soff) noexcept {

    auto const find_conv_impl = [](RuntimeConversionTable const &lesser, RuntimeConversionTable const &greater,
                                   size_t const lesser_init_soff, size_t const greater_init_soff) noexcept -> std::optional<DatatypeConverter> {
        auto const lesser_s_rank = lesser.subtype_rank() - lesser_init_soff;
        auto const greater_s_rank = greater.subtype_rank() - greater_init_soff;

        // lesser should be the conversion table of the type with lower subtype rank
        RDF4CPP_ASSERT(lesser_s_rank <= greater_s_rank);

        // calculate initial subtype offsets to equalize subtype rank
        size_t lesser_s_off = lesser_init_soff;
        size_t greater_s_off = greater_init_soff + greater_s_rank - lesser_s_rank;

        while (lesser_s_off < lesser.subtype_rank() && greater_s_off < greater.subtype_rank()) {
            auto const lesser_p_rank = lesser.promotion_rank_at_level(lesser_s_off);
            auto const greater_p_rank = greater.promotion_rank_at_level(greater_s_off);

            if (lesser_p_rank == greater_p_rank) {
                // subtype rank and promotion rank equal
                // => potential for correct conversion

                RuntimeConversionEntry const &lconv = lesser.conversion_at_index(lesser_s_off, 0);
                RuntimeConversionEntry const &gconv = greater.conversion_at_index(greater_s_off, 0);

                if (lconv.target_type_id == gconv.target_type_id) {
                    // correct conversion found
                    return DatatypeConverter::from_individuals(lconv, gconv);
                }
            } else {
                // subtype rank equal
                // promotion rank not yet equal

                // calculate promotion offsets to equalize promotion rank
                auto const [lesser_p_off, greater_p_off] = lesser_p_rank < greater_p_rank
                                                                   ? std::make_pair(0ul, greater_p_rank - lesser_p_rank)
                                                                   : std::make_pair(lesser_p_rank - greater_p_rank, 0ul);

                if (lesser_p_off < lesser_p_rank && greater_p_off < greater_p_rank) {
                    RuntimeConversionEntry const &lconv = lesser.conversion_at_index(lesser_s_off, lesser_p_off);
                    RuntimeConversionEntry const &gconv = greater.conversion_at_index(greater_s_off, greater_p_off);

                    if (lconv.target_type_id == gconv.target_type_id) {
                        // correct conversion found
                        return DatatypeConverter::from_individuals(lconv, gconv);
                    }
                }
            }

            lesser_s_off += 1;
            greater_s_off += 1;
        }

        // no conversion available
        return std::nullopt;
    };

    // call find_conv_impl with entries in correct order (lesser s rank, greater s rank)
    if (lhs_conv.subtype_rank() - lhs_init_soff < rhs_conv.subtype_rank() - rhs_init_soff) {
        return find_conv_impl(lhs_conv, rhs_conv, lhs_init_soff, rhs_init_soff);
    } else {
        auto res = find_conv_impl(rhs_conv, lhs_conv, rhs_init_soff, lhs_init_soff);

        if (res.has_value()) {
            // swap functions to reverse the ordering change
            std::swap(res->convert_lhs, res->convert_rhs);
            std::swap(res->inverted_convert_lhs, res->inverted_convert_rhs);
        }

        return res;
    }
}

} // namespace rdf4cpp::datatypes::registry
