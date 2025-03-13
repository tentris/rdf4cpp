#ifndef RDF4CPP_RDF_STORAGE_REFERENCENODESTORAGE_SPECIALIZEDLITERALBACKEND_HPP
#define RDF4CPP_RDF_STORAGE_REFERENCENODESTORAGE_SPECIALIZEDLITERALBACKEND_HPP

#include <rdf4cpp/storage/identifier/NodeID.hpp>
#include <rdf4cpp/storage/view/LiteralBackendView.hpp>

namespace rdf4cpp::storage::reference_node_storage {

template<datatypes::FixedIdLiteralDatatype T>
struct SpecializedLiteralBackend {
    using view_type = view::ValueLiteralBackendView;
    using literal_type = T;
    using id_type = identifier::LiteralID;

    size_t hash;
    static constexpr identifier::LiteralType datatype = T::fixed_id;
    typename T::cpp_type value;

    explicit SpecializedLiteralBackend(view_type const &view) noexcept : hash{view.hash<literal_type>()},
                                                                         value{std::any_cast<typename T::cpp_type>(view.value)} {
        assert(view.datatype == SpecializedLiteralBackend::datatype);
    }

    explicit operator view_type() const noexcept {
        return view_type{.datatype = datatype,
                         .value = value};
    }

    /**
     * Translates the given id_type (= LiteralID) into a NodeID by attaching the
     * LiteralType from the given view to it.
     */
    static identifier::NodeBackendID from_storage_id(id_type const id, view_type const &view) noexcept {
        return identifier::NodeBackendID{identifier::NodeID{id, view.datatype}, identifier::RDFNodeType::Literal};
    }

    /**
     * Translates the given NodeID into an id_type (= LiteralID) by extracting the LiteralID
     */
    static id_type to_storage_id(identifier::NodeBackendID const id) noexcept {
        return id.node_id().literal_id();
    }

    struct equal {
        using is_transparent = void;

        bool operator()(SpecializedLiteralBackend const &lhs, SpecializedLiteralBackend const &rhs) const noexcept {
            return lhs.value == rhs.value;
        }

        bool operator()(view_type const &lhs, SpecializedLiteralBackend const &rhs) const noexcept {
            assert(lhs.datatype == SpecializedLiteralBackend::datatype);
            return lhs.eq<literal_type>(rhs.value);
        }

        bool operator()(SpecializedLiteralBackend const &lhs, view_type const &rhs) const noexcept {
            assert(SpecializedLiteralBackend::datatype == rhs.datatype);
            return rhs.eq<literal_type>(lhs.value);
        }
    };

    struct hasher {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(view_type const &x) const noexcept {
            assert(x.datatype == SpecializedLiteralBackend::datatype);
            return x.hash<literal_type>();
        }
    };

    [[nodiscard]] static view_type get_default_view() noexcept {
        return view_type{.datatype = T::fixed_id, .value = typename T::cpp_type{}};
    }
};

}  // namespace rdf4cpp::storage::reference_node_storage

#endif  //RDF4CPP_RDF_STORAGE_REFERENCENODESTORAGE_SPECIALIZEDLITERALBACKEND_HPP
