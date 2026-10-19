#include <rdf4cpp/util/Anonymizer.hpp>
#include <rdf4cpp/IRIFactory.hpp>

#include <ranges>
#include <format>

#include <botan/auto_rng.h>

namespace rdf4cpp::util {

inline constexpr std::array chars{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
                                  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                  'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                                  'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

struct GlobalCryptoRng {
    using result_type = uint64_t;

    constexpr GlobalCryptoRng() noexcept = default;

    [[nodiscard]] result_type operator()() const {
        thread_local Botan::AutoSeeded_RNG rng;
        return std::bit_cast<result_type>(rng.template random_array<sizeof(result_type)>());
    }

    static constexpr result_type min() noexcept {
        return std::numeric_limits<result_type>::min();
    }

    static constexpr result_type max() noexcept {
        return std::numeric_limits<result_type>::max();
    }
};
inline constexpr GlobalCryptoRng global_crypto_rng;


Anonymizer::Anonymizer(storage::DynNodeStoragePtr node_storage)
    : dist_{0, chars.size() - 1},
      node_storage_{node_storage} {
}

void Anonymizer::gen_random_id(std::span<char> out) {
    std::ranges::generate(out, [this] {
        return chars[dist_(global_crypto_rng)];
    });
}

storage::DynNodeStoragePtr Anonymizer::node_storage() const noexcept {
    return node_storage_;
}

template<datatypes::registry::util::ConstexprString base>
Anonymizer::MkAnon<base>::MkAnon(Anonymizer &parent) noexcept : parent_{&parent} {
    std::ranges::copy(base.view(), buf_.begin());
}

template<datatypes::registry::util::ConstexprString base>
std::string_view Anonymizer::MkAnon<base>::operator()() {
    parent_->gen_random_id(std::span{buf_}.subspan(base.size()));
    return std::string_view{buf_.data(), buf_.size()};
}

Literal Anonymizer::anonymize_impl(Literal const &lit) {
    RDF4CPP_ASSERT(!lit.null());
    MkAnon<"http://example.org/"> mk_anon{*this};

    if (lit.template datatype_eq<datatypes::xsd::String>()) {
        return Literal::make_simple(mk_anon(), node_storage_);
    }
    if (lit.template datatype_eq<datatypes::rdf::LangString>()) {
        return Literal::make_lang_tagged(mk_anon(), lit.language_tag(), node_storage_);
    }

    // Simplification: things without intrinsic meaning are kept as they are
    // note: the partial dates (gYear, ...) are not tagged as timepoints
    if (lit.is_numeric()
        || lit.is_duration()
        || lit.is_timepoint()
        || lit.template datatype_eq<datatypes::xsd::Boolean>()
        || lit.template datatype_eq<datatypes::xsd::GYear>()
        || lit.template datatype_eq<datatypes::xsd::GMonth>()
        || lit.template datatype_eq<datatypes::xsd::GDay>()
        || lit.template datatype_eq<datatypes::xsd::GYearMonth>()
        || lit.template datatype_eq<datatypes::xsd::GMonthDay>()) {
        return lit;
    }

    if (lit.template datatype_eq<datatypes::xsd::HexBinary>()) {
        auto const str = mk_anon();
        datatypes::xsd::HexBinary::cpp_type hexb;
        std::ranges::copy(as_bytes(std::span{str}), std::back_inserter(hexb.bytes));

        return Literal::make_typed_from_value<datatypes::xsd::HexBinary>(hexb, node_storage_);
    }

    if (lit.template datatype_eq<datatypes::xsd::Base64Binary>()) {
        auto const str = mk_anon();
        datatypes::xsd::Base64Binary::cpp_type b64;
        std::ranges::copy(as_bytes(std::span{str}), std::back_inserter(b64.bytes));

        return Literal::make_typed_from_value<datatypes::xsd::Base64Binary>(b64, node_storage_);
    }

    // Note: only unknown datatypes and anyURI left, we don't need to worry about parsability (mk_anon_str() returns IRI string)
    return Literal::make_typed(mk_anon(), lit.datatype(), node_storage_);
}

BlankNode Anonymizer::anonymize_impl([[maybe_unused]] BlankNode const &bnode) {
    RDF4CPP_ASSERT(!bnode.null());
    MkAnon<"bn_"> mk_anon{*this};
    return BlankNode::make(mk_anon(), node_storage_);
}

IRI Anonymizer::anonymize_impl(IRI const &iri) {
    RDF4CPP_ASSERT(!iri.null());

    if (datatypes::registry::DatatypeRegistry::get_entry(datatypes::registry::DatatypeIDView{iri}) != nullptr) {
        return iri;
    }

    MkAnon<"http://example.org/"> mk_anon_str{*this};
    return IRI::make(mk_anon_str(), node_storage_);
}

query::Variable Anonymizer::anonymize_impl(query::Variable const &var) {
    RDF4CPP_ASSERT(!var.null());
    MkAnon<"var_"> mk_anon{*this};

    if (var.is_anonymous()) {
        return query::Variable::make_anonymous(mk_anon(), node_storage_);
    }

    return query::Variable::make_named(mk_anon(), node_storage_);
}

Node Anonymizer::anonymize(Node const &node) {
    using namespace storage::identifier;

    Node ret;

    // Intern into our own node storage to ensure the lookup works
    // for terms with same content in different node storages.
    Node const non_anon = node.to_node_storage(node_storage_);

    if (non_anon.null() || non_anon.as_iri().is_default_graph()) {
        ret = non_anon.as_iri();
        return ret;
    }

    if (auto it = lookup_.find(non_anon.backend_handle()); it != lookup_.end()) {
        ret = Node{NodeBackendHandle{it->second, node_storage_}};
        return ret;
    }

    ret = [&] -> Node {
        switch (non_anon.backend_handle().id().type()) {
            using enum RDFNodeType;
            case IRI: {
                return anonymize_impl(non_anon.as_iri());
            }
            case BNode: {
                return anonymize_impl(non_anon.as_blank_node());
            }
            case Literal: {
                return anonymize_impl(non_anon.as_literal());
            }
            case Variable: {
                return anonymize_impl(non_anon.as_variable());
            }
            default: {
                RDF4CPP_UNREACHABLE;
            }
        }
    }();

    lookup_.emplace(non_anon.backend_handle(), ret.backend_handle().id());
    return ret;
}

Statement Anonymizer::anonymize(Statement const &non_anon) {
    return Statement{
        anonymize(non_anon.subject()),
        anonymize(non_anon.predicate()),
        anonymize(non_anon.object())
    };
}

Quad Anonymizer::anonymize(Quad const &non_anon) {
    return Quad{
        anonymize(non_anon.graph()),
        anonymize(non_anon.subject()),
        anonymize(non_anon.predicate()),
        anonymize(non_anon.object())
    };
}

} // namespace rdf4cpp::util