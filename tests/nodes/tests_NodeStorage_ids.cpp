#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
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
        check_string_repr(LiteralID{123}, "LiteralID { .underlying = 123 }");
    }

    TEST_CASE("LiteralTypeTag") {
        check_string_repr(LiteralTypeTag::Default, "Default");
        check_string_repr(LiteralTypeTag::Duration, "Duration");
        check_string_repr(LiteralTypeTag::Timepoint, "Timepoint");
        check_string_repr(LiteralTypeTag::Numeric, "Numeric");
    }

    TEST_CASE("LiteralType") {
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Default, 1), "LiteralType { .tag = Default, .id = 1 }");
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Duration, 1), "LiteralType { .tag = Duration, .id = 1 }");
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Timepoint, 1), "LiteralType { .tag = Timepoint, .id = 1 }");
        check_string_repr(LiteralType::from_parts(LiteralTypeTag::Numeric, 1), "LiteralType { .tag = Numeric, .id = 1 }");
    }

    TEST_CASE("NodeID") {
        check_string_repr(NodeID{LiteralID{1}, LiteralType::from_parts(LiteralTypeTag::Default, 1)}, "NodeID { .underlying = 4398046511105 }");
        check_string_repr(NodeID{123}, "NodeID { .underlying = 123 }");
    }

    TEST_CASE("NodeBackendID") {
        check_string_repr(NodeBackendID{NodeID{LiteralID{1}, LiteralType::from_parts(LiteralTypeTag::Default, 1)}, RDFNodeType::Literal},
                          "NodeBackendID { .node_id = NodeID { .literal_id = LiteralID { .underlying = 1 }, .literal_type = LiteralType { .tag = Default, .id = 1 } }, .type = Literal, .is_inlined = false, .free_tagging_bits = 0 }");
        check_string_repr(NodeBackendID{NodeID{123}, RDFNodeType::IRI},
                          "NodeBackendID { .node_id = NodeID { .underlying = 123 }, .type = IRI, .is_inlined = false, .free_tagging_bits = 0 }");
    }

    TEST_CASE("NodeBackendHandle") {
        check_string_repr(NodeBackendHandle{NodeBackendID{NodeID{123}, RDFNodeType::IRI}, nullptr},
                          "NodeBackendHandle { .id = NodeBackendID { .node_id = NodeID { .underlying = 123 }, .type = IRI, .is_inlined = false, .free_tagging_bits = 0 }, .node_storage = DynNodeStoragePtr { .backend = 0, .vtable = 0 } }");
        check_string_repr(NodeBackendHandle{NodeBackendID{NodeID{123}, RDFNodeType::IRI}, rdf4cpp::storage::default_node_storage},
                          std::format("NodeBackendHandle {{ .id = NodeBackendID {{ .node_id = NodeID {{ .underlying = 123 }}, .type = IRI, .is_inlined = false, .free_tagging_bits = 0 }}, .node_storage = DynNodeStoragePtr {{ .backend = {}, .vtable = {} }} }}",
                              rdf4cpp::storage::default_node_storage.backend(), static_cast<void const *>(rdf4cpp::storage::default_node_storage.vtable())));
    }

}