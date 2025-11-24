#include <rdf4cpp/util/Anonymizer.hpp>
#include <rdf4cpp/IRIFactory.hpp>

#include <ranges>
#include <format>

#include <openssl/err.h>
#include <openssl/rand.h>

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
        std::array<unsigned char, sizeof(result_type)> buf;

        int const ret = RAND_bytes(buf.data(), buf.size());
        if (ret == 0) [[unlikely]] {
            std::array<char, 120> err_buf;
            unsigned long const code = ERR_get_error();
            ERR_error_string_n(code, err_buf.data(), err_buf.size());

            throw std::runtime_error{std::format("Unable to generate random bytes for anonymizer: {}", std::string_view{err_buf.data()})};
        }

        return std::bit_cast<result_type>(buf);
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