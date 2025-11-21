#include <rdf4cpp/util/Anonymizer.hpp>
#include <rdf4cpp/IRIFactory.hpp>

#include <ranges>
#include <format>

namespace rdf4cpp::util {

inline constexpr std::array chars{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
                                  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                  'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                                  'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

Anonymizer::Anonymizer(storage::DynNodeStoragePtr node_storage, std::optional<uint64_t> seed)
    : rng_{seed.has_value() ? *seed : std::random_device{}()},
      dist_{0, chars.size() - 1},
      node_storage_{node_storage} {
}

void Anonymizer::gen_random_id(std::span<char> out) {
    std::ranges::generate(out, [this] {
        return chars[dist_(rng_)];
    });
}

storage::DynNodeStoragePtr Anonymizer::node_storage() const noexcept {
    return node_storage_;
}

IRI Anonymizer::anonymize(Node const &non_anon) {
    using namespace storage::identifier;

    if (non_anon.null() || non_anon.as_iri().is_default_graph()) {
        return non_anon.as_iri();
    }

    if (auto it = lookup_.find(non_anon.backend_handle()); it != lookup_.end()) {
        return IRI{NodeBackendHandle{it->second, node_storage_}};
    }

    std::array<char, IRIFactory::default_base.size() + 16> buf;
    std::ranges::copy(IRIFactory::default_base, buf.begin());
    gen_random_id(std::span{buf}.subspan(IRIFactory::default_base.size()));

    auto anon = IRI::make(std::string_view{buf.data(), buf.size()}, node_storage_);
    lookup_.emplace(non_anon.backend_handle(), anon.backend_handle().id());

    return anon;
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