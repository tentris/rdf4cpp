#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp"


#include <doctest/doctest.h>

#include <rdf4cpp/storage/identifier/NodeBackendHandle.hpp>

TEST_SUITE("node storage identifier output") {
    using namespace rdf4cpp::storage::identifier;

    template<typename T>
    void check_string_repr(T id, std::string_view expected) {
        std::ostringstream oss;
        oss << id;
        CHECK_EQ(oss.str(), expected);
    }

    TEST_CASE("RDFNodeType") {
        check_string_repr(RDFNodeType::Literal, "Literal");
        check_string_repr(RDFNodeType::Variable, "Variable");
        check_string_repr(RDFNodeType::BNode, "BNode");
        check_string_repr(RDFNodeType::IRI, "IRI");
    }

    TEST_CASE("LiteralID") {
        check_string_repr(LiteralID{123}, "{ .underlying = 123 }");
    }

    TEST_CASE("LiteralTypeTag") {
        check_string_repr(LiteralTypeTag::Default, "Default");
        check_string_repr(LiteralTypeTag::Duration, "Duration");
        check_string_repr(LiteralTypeTag::Timepoint, "Timepoint");
        check_string_repr(LiteralTypeTag::Numeric, "Numeric");
    }

    TEST_CASE("LiteralType") {
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Default, 1), "{ .tag = Default, .id = 1 }");
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Duration, 1), "{ .tag = Duration, .id = 1 }");
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Timepoint, 1), "{ .tag = Timepoint, .id = 1 }");
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Numeric, 1), "{ .tag = Numeric, .id = 1 }");
    }

    TEST_CASE("NodeID") {
        check_string_repr(NodeID{LiteralID{1}, LiteralType::from_parts(LiteralTypeTag::Default, 1)},
                          std::format("{{ .underlying = {} }}", (1UL << LiteralID::width) + 1));
        check_string_repr(NodeID{123}, "{ .underlying = 123 }");
    }

    TEST_CASE("NodeBackendID") {
        check_string_repr(NodeBackendID{NodeID{LiteralID{1}, LiteralType::from_parts(LiteralTypeTag::Default, 1)}, RDFNodeType::Literal},
                          "{ .node_id = { .literal_id = { .underlying = 1 }, .literal_type = { .tag = Default, .id = 1 } }, .type = Literal, .is_inlined = false, .free_tagging_bits = 0 }");
        check_string_repr(NodeBackendID{NodeID{123}, RDFNodeType::IRI},
                          "{ .node_id = { .underlying = 123 }, .type = IRI, .is_inlined = false, .free_tagging_bits = 0 }");
    }

    TEST_CASE("NodeBackendHandle") {
        check_string_repr(NodeBackendHandle{NodeBackendID{NodeID{123}, RDFNodeType::IRI}, nullptr},
                          "{ .id = { .node_id = { .underlying = 123 }, .type = IRI, .is_inlined = false, .free_tagging_bits = 0 }, .node_storage = { .backend = 0, .vtable = 0 } }");
        // the pointers have to be stringified the same way the printer does it,
        // std::format and operator<< are not required to spell them alike (e.g. 0x0 vs 0)
        auto const ns = rdf4cpp::storage::default_node_storage;
        std::ostringstream expected;
        expected << "{ .id = { .node_id = { .underlying = 123 }, .type = IRI, .is_inlined = false, .free_tagging_bits = 0 }, .node_storage = { .backend = "
                 << ns.backend() << ", .vtable = " << ns.vtable() << " } }";
        check_string_repr(NodeBackendHandle{NodeBackendID{NodeID{123}, RDFNodeType::IRI}, ns}, expected.str());
    }

    TEST_CASE("Address (in)equality") {
        using namespace rdf4cpp::storage::view;
        // wraps the backing storage at the same address, forwarding everything the concept requires
        struct TestNs {
            rdf4cpp::storage::reference_node_storage::UnsyncReferenceNodeStorage backing;

            static bool has_specialized_storage_for(LiteralType t) noexcept { return decltype(backing)::has_specialized_storage_for(t); }

            NodeBackendID find_or_make_id(BNodeBackendView const &v) { return backing.find_or_make_id(v); }
            NodeBackendID find_or_make_id(IRIBackendView const &v) { return backing.find_or_make_id(v); }
            NodeBackendID find_or_make_id(LiteralBackendView const &v) { return backing.find_or_make_id(v); }
            NodeBackendID find_or_make_id(VariableBackendView const &v) { return backing.find_or_make_id(v); }

            NodeBackendID find_id(BNodeBackendView const &v) const noexcept { return backing.find_id(v); }
            NodeBackendID find_id(IRIBackendView const &v) const noexcept { return backing.find_id(v); }
            NodeBackendID find_id(LiteralBackendView const &v) const noexcept { return backing.find_id(v); }
            NodeBackendID find_id(VariableBackendView const &v) const noexcept { return backing.find_id(v); }

            IRIBackendView find_iri_backend(NodeBackendID id) const noexcept { return backing.find_iri_backend(id); }
            LiteralBackendView find_literal_backend(NodeBackendID id) const noexcept { return backing.find_literal_backend(id); }
            BNodeBackendView find_bnode_backend(NodeBackendID id) const noexcept { return backing.find_bnode_backend(id); }
            VariableBackendView find_variable_backend(NodeBackendID id) const noexcept { return backing.find_variable_backend(id); }
        };
        static_assert(rdf4cpp::storage::NodeStorage<TestNs>);

        TestNs ns;
        CHECK_EQ(static_cast<void const *>(&ns), static_cast<void const *>(&ns.backing)); // both live at the same address
        CHECK_NE(rdf4cpp::storage::DynNodeStoragePtr{ns}, rdf4cpp::storage::DynNodeStoragePtr{ns.backing}); // but comparison can differentiate the two
    }
}
