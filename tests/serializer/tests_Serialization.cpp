#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>

#include <cmath>

enum struct OutputFormat {
    NTriples,
    Turtle,
    NQuads,
    TriG
};

template<OutputFormat F>
concept format_has_graph = (F == OutputFormat::NQuads || F == OutputFormat::TriG);

template<OutputFormat F>
concept format_has_prefix = (F == OutputFormat::Turtle || F == OutputFormat::TriG);

using namespace rdf4cpp;

TEST_CASE("Literal short form and prefixed") {
    std::string buf;
    writer::StringWriter ser{buf};
    Literal::make_typed<datatypes::xsd::Date>("2042-5-4").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Boolean>("true").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Decimal>("4").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Double>("4").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Double>("INF").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Double>("-INF").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Double>("NaN").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed_from_value<datatypes::xsd::Double>(HUGE_VAL).serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Float>("4").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::UnsignedByte>("4").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Integer>("4").serialize(ser, NodeSerializationOpts::prefixed_and_short_form());
    ser.finalize();

    CHECK_EQ(buf, R"("2042-05-04"^^xsd:date,true,4.0,4.0E0,"INF"^^xsd:double,"-INF"^^xsd:double,"NaN"^^xsd:double,"INF"^^xsd:double,"4.0E0"^^xsd:float,"4"^^xsd:unsignedByte,4)");
}

TEST_CASE("Literal short form") {
    std::string buf;
    writer::StringWriter ser{buf};
    Literal::make_typed<datatypes::xsd::Date>("2042-5-4").serialize(ser, NodeSerializationOpts::short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Boolean>("true").serialize(ser, NodeSerializationOpts::short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Decimal>("4").serialize(ser, NodeSerializationOpts::short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Double>("4").serialize(ser, NodeSerializationOpts::short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Float>("4").serialize(ser, NodeSerializationOpts::short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::UnsignedByte>("4").serialize(ser, NodeSerializationOpts::short_form());
    writer::write_str(",", ser);
    Literal::make_typed<datatypes::xsd::Integer>("4").serialize(ser, NodeSerializationOpts::short_form());
    ser.finalize();

    CHECK_EQ(buf, R"("2042-05-04"^^<http://www.w3.org/2001/XMLSchema#date>,true,4.0,4.0E0,"4.0E0"^^<http://www.w3.org/2001/XMLSchema#float>,"4"^^<http://www.w3.org/2001/XMLSchema#unsignedByte>,4)");
}

template<OutputFormat F>
bool serialize(Quad const &q, writer::BufWriterParts writer, writer::SerializationState* state) {
    if constexpr (F == OutputFormat::NTriples)
        return q.serialize_ntriples(writer);
    else if constexpr (F == OutputFormat::NQuads)
        return q.serialize_nquads(writer);
    else if constexpr (F == OutputFormat::Turtle)
        return q.serialize_turtle(*state, writer);
    else if constexpr (F == OutputFormat::TriG)
        return q.serialize_trig(*state, writer);
}

[[nodiscard]] std::array<Quad, 3> quads(storage::DynNodeStoragePtr storage = storage::default_node_storage) {
    return {
            Quad{IRI::default_graph(storage), IRI::make("http://ex/sub", storage), IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", storage), Literal::make_typed_from_value<datatypes::xsd::Int>(7, storage)},
            Quad{IRI::make("http://ex/graph", storage), IRI::make("http://ex/sub", storage), IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", storage), IRI::make("http://ex/obj")},
            Quad{IRI::make("http://ex/graph2", storage), IRI::make("http://ex/sub", storage), IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type", storage), Literal::make_typed_from_value<datatypes::xsd::Int>(5, storage)}};
}

template<OutputFormat F>
std::string write_basic_data(){
    std::string buf;
    writer::StringWriter ser{buf};
    writer::SerializationState st{};

    if constexpr (format_has_prefix<F>) {
        if (!writer::SerializationState::begin(ser)) {
            FAIL("state failed");
        }
    }

    for (auto const &q : quads()) {
        if (!serialize<F>(q, ser, &st)) {
            FAIL("write failed");
        }
    }

    if constexpr (format_has_prefix<F>) {
        if (!st.flush(ser)) {
            FAIL("flush failed");
        }
    }

    ser.finalize();
    return buf;
}
Graph get_graph(storage::DynNodeStoragePtr node_storage) {
    Graph gd{node_storage};
    for (auto const &q : quads(node_storage)) {
        gd.add(q.without_graph());
    }
    return gd;
}
Dataset get_dataset(storage::DynNodeStoragePtr node_storage) {
    Dataset gd{node_storage};
    for (auto const &q : quads(node_storage)) {
        gd.add(q);
    }
    return gd;
}

template<bool HasGraph, parser::ParsingFlag F>
void check_basic_data(const std::string &i) {
    using namespace parser;
    std::istringstream iss{i};
    IStreamQuadIterator qit{iss, F | ParsingFlag::KeepBlankNodeIds};
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"));
    CHECK(qit->value().object() == Literal::make_typed_from_value<datatypes::xsd::Int>(7));
    if constexpr (HasGraph) {
        CHECK(qit->value().graph() == IRI::default_graph());
    } else {
        CHECK(qit->value().graph() == IRI::default_graph());
    }

    ++qit;
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"));
    CHECK(qit->value().object() == IRI::make("http://ex/obj"));
    if constexpr (HasGraph) {
        CHECK(qit->value().graph() == IRI::make("http://ex/graph"));
    } else {
        CHECK(qit->value().graph() == IRI::default_graph());
    }

    ++qit;
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"));
    CHECK(qit->value().object() == Literal::make_typed_from_value<datatypes::xsd::Int>(5));
    if constexpr (HasGraph) {
        CHECK(qit->value().graph() == IRI::make("http://ex/graph2"));
    } else {
        CHECK(qit->value().graph() == IRI::default_graph());
    }

    ++qit;
    CHECK(qit == std::default_sentinel);
}

TEST_CASE("basic ntriple") {
    std::string const d = write_basic_data<OutputFormat::NTriples>();
    check_basic_data<false, parser::ParsingFlag::NTriples>(d);

    CHECK_EQ(d,
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"7\"^^<http://www.w3.org/2001/XMLSchema#int> .\n"
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://ex/obj> .\n"
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"5\"^^<http://www.w3.org/2001/XMLSchema#int> .\n");

    auto const res = writer::StringWriter::oneshot([](auto &w) noexcept {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns{};
        return get_graph(ns).serialize(w);
    });

    // reordered because graph is unordered_map
    CHECK_EQ(res,
            "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"5\"^^<http://www.w3.org/2001/XMLSchema#int> .\n"
            "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://ex/obj> .\n"
            "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"7\"^^<http://www.w3.org/2001/XMLSchema#int> .\n"
            );
}

TEST_CASE("basic nquad") {
    std::string const d = write_basic_data<OutputFormat::NQuads>();
    check_basic_data<true, parser::ParsingFlag::NQuads>(d);

    CHECK_EQ(d,
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"7\"^^<http://www.w3.org/2001/XMLSchema#int> .\n"
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://ex/obj> <http://ex/graph> .\n"
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"5\"^^<http://www.w3.org/2001/XMLSchema#int> <http://ex/graph2> .\n");

    auto const res = writer::StringWriter::oneshot([](auto &w) noexcept {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns{};
        return get_dataset(ns).serialize(w);
    });

    // reordered because dataset is unordered_map
    CHECK_EQ(res,
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"5\"^^<http://www.w3.org/2001/XMLSchema#int> <http://ex/graph2> .\n"
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://ex/obj> <http://ex/graph> .\n"
             "<http://ex/sub> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> \"7\"^^<http://www.w3.org/2001/XMLSchema#int> .\n");
}

TEST_CASE("basic turtle") {
    std::string const d = write_basic_data<OutputFormat::Turtle>();
    check_basic_data<false, parser::ParsingFlag::Turtle>(d);

    CHECK_EQ(d,
             "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
             "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
             "<http://ex/sub> a \"7\"^^xsd:int ,\n"
             "<http://ex/obj> ,\n"
             "\"5\"^^xsd:int .\n");

    auto const res = writer::StringWriter::oneshot([](auto &w) noexcept {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns{};
        return get_graph(ns).serialize_turtle(w);
    });

    // reordered because graph is unordered_map
    CHECK_EQ(res,
             "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
             "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
             "<http://ex/sub> a \"5\"^^xsd:int ,\n"
             "<http://ex/obj> ,\n"
             "\"7\"^^xsd:int .\n");
}

TEST_CASE("basic trig") {
    const std::string d = write_basic_data<OutputFormat::TriG>();
    check_basic_data<true, parser::ParsingFlag::TriG>(d);

    CHECK_EQ(d,
             "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
             "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
             "<http://ex/sub> a \"7\"^^xsd:int .\n"
             "<http://ex/graph> {\n"
             "<http://ex/sub> a <http://ex/obj> .\n"
             "}\n"
             "<http://ex/graph2> {\n"
             "<http://ex/sub> a \"5\"^^xsd:int .\n"
             "}\n");

    auto const res = writer::StringWriter::oneshot([](auto &w) noexcept {
        storage::reference_node_storage::UnsyncReferenceNodeStorage ns{};
        return get_dataset(ns).serialize_trig(w);
    });

    // reordered because dataset is unordered_map
    CHECK_EQ(res,
             "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n"
             "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
             "<http://ex/graph2> {\n"
             "<http://ex/sub> a \"5\"^^xsd:int .\n"
             "}\n"
             "<http://ex/graph> {\n"
             "<http://ex/sub> a <http://ex/obj> .\n"
             "}\n"
             "<http://ex/sub> a \"7\"^^xsd:int .\n");
}

TEST_CASE("nquads bnode graph") {
    Quad q{
        BlankNode::make("G"),
        IRI::make("http://example.com#s"),
        IRI::make("http://example.com#p"),
        IRI::make("http://example.com#o"),
    };

    auto s = writer::StringWriter::oneshot([&q](auto &w) {
        q.serialize_nquads(w);
        return true;
    });

    CHECK_EQ(s, "<http://example.com#s> <http://example.com#p> <http://example.com#o> _:G .\n");
}

TEST_CASE("trig bnode graph") {
    Quad q{
        BlankNode::make("G"),
        IRI::make("http://example.com#s"),
        IRI::make("http://example.com#p"),
        IRI::make("http://example.com#o"),
    };

    auto s = writer::StringWriter::oneshot([&q](auto &w) {
        writer::SerializationState st{};
        st.begin(w);
        q.serialize_trig(st, w);
        st.flush(w);
        return true;
    });

    CHECK_EQ(s, R"(@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .
_:G {
<http://example.com#s> <http://example.com#p> <http://example.com#o> .
}
)");
}

template<OutputFormat F>
std::string write_ext_data() {
    std::string buf;
    writer::StringWriter ser{buf};
    writer::SerializationState st{};
    if constexpr (format_has_prefix<F>) {
        if (!writer::SerializationState::begin(ser)) {
            FAIL("state failed");
        }
    }

    Quad q{IRI::make("http://ex/graph"), IRI::make("http://ex/sub"), IRI::make("http://ex/pred"), IRI::make("http://ex/obj")};
    if (!serialize<F>(q, ser, &st)) {
        FAIL("write failed");
    }

    q.object() = Literal::make_typed_from_value<datatypes::xsd::Integer>(5);
    if (!serialize<F>(q, ser, &st)) {
        FAIL("write failed");
    }

    q.object() = Literal::make_typed_from_value<datatypes::xsd::Double>(5.0);
    if (!serialize<F>(q, ser, &st)) {
        FAIL("write failed");
    }

    q.object() = Literal::make_typed<datatypes::xsd::Decimal>("4.2");
    if (!serialize<F>(q, ser, &st)) {
        FAIL("write failed");
    }

    q.predicate() = IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
    q.object() = Literal::make_typed_from_value<datatypes::xsd::Boolean>(true);
    if (!serialize<F>(q, ser, &st)) {
        FAIL("write failed");
    }

    if constexpr (format_has_prefix<F>) {
        if (!st.flush(ser)) {
            FAIL("flush failed");
        }
    }

    ser.finalize();
    return buf;
}
template<parser::ParsingFlag F>
void check_ext_data(const std::string &i) {
    using namespace parser;
    std::istringstream iss{i};
    IStreamQuadIterator qit{iss, F | ParsingFlag::KeepBlankNodeIds};

    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://ex/pred"));
    CHECK(qit->value().object() == IRI::make("http://ex/obj"));
    if constexpr (F == parser::ParsingFlag::TriG)
        CHECK(qit->value().graph() == IRI::make("http://ex/graph"));
    else
        CHECK(qit->value().graph() == IRI::default_graph());

    ++qit;
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://ex/pred"));
    CHECK(qit->value().object() == Literal::make_typed_from_value<datatypes::xsd::Integer>(5));
    if constexpr (F == parser::ParsingFlag::TriG)
        CHECK(qit->value().graph() == IRI::make("http://ex/graph"));
    else
        CHECK(qit->value().graph() == IRI::default_graph());

    ++qit;
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://ex/pred"));
    CHECK(qit->value().object() == Literal::make_typed_from_value<datatypes::xsd::Double>(5.0));
    if constexpr (F == parser::ParsingFlag::TriG)
        CHECK(qit->value().graph() == IRI::make("http://ex/graph"));
    else
        CHECK(qit->value().graph() == IRI::default_graph());

    ++qit;
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://ex/pred"));
    CHECK(qit->value().object() == Literal::make_typed<datatypes::xsd::Decimal>("4.2"));
    if constexpr (F == parser::ParsingFlag::TriG)
        CHECK(qit->value().graph() == IRI::make("http://ex/graph"));
    else
        CHECK(qit->value().graph() == IRI::default_graph());

    ++qit;
    CHECK(qit != std::default_sentinel);
    CHECK(qit->value().subject() == IRI::make("http://ex/sub"));
    CHECK(qit->value().predicate() == IRI::make("http://www.w3.org/1999/02/22-rdf-syntax-ns#type"));
    CHECK(qit->value().object() == Literal::make_typed_from_value<datatypes::xsd::Boolean>(true));
    if constexpr (F == parser::ParsingFlag::TriG)
        CHECK(qit->value().graph() == IRI::make("http://ex/graph"));
    else
        CHECK(qit->value().graph() == IRI::default_graph());

    ++qit;
    CHECK(qit == std::default_sentinel);
}
template<OutputFormat F, parser::ParsingFlag I>
void extended_tests() {
    std::string data = write_ext_data<F>();
    if constexpr (F == OutputFormat::Turtle) {
        CHECK(data == "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n<http://ex/sub> <http://ex/pred> <http://ex/obj> ,\n5 ,\n5.0E0 ,\n4.2 ;\na true .\n");
    } else {
        CHECK(data == "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n<http://ex/graph> {\n<http://ex/sub> <http://ex/pred> <http://ex/obj> ,\n5 ,\n5.0E0 ,\n4.2 ;\na true .\n}\n");
    }

    check_ext_data<I>(data);
}
TEST_CASE("extended") {
    // ntriples & nquads don't support any of the syntax tested here
    extended_tests<OutputFormat::Turtle, parser::ParsingFlag::Turtle>();
    extended_tests<OutputFormat::TriG, parser::ParsingFlag::TriG>();
}

static_assert(datatypes::registry::util::ConstexprString("abc")+datatypes::registry::util::ConstexprString("def") == datatypes::registry::util::ConstexprString("abcdef"));
static_assert((datatypes::registry::util::ConstexprString("abc")+datatypes::registry::util::ConstexprString("def")).size() == 6);
