#include <rdf4cpp/bnode_mngt/reference_backends/generator/RandomIdGenerator.hpp>

#include <array>
#include <algorithm>

namespace rdf4cpp::bnode_mngt {

namespace generator_detail {

static constexpr std::array bnode_id_valid_chars{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
                                                 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                                 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                                 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                                                 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

static constexpr size_t generated_id_size = 32;

}  //namespace generator_detail

RandomIdGenerator::RandomIdGenerator() : RandomIdGenerator{std::random_device{}()} {
}

RandomIdGenerator::RandomIdGenerator(uint64_t const seed) : rng_{seed},
                                                            dist_{0, generator_detail::bnode_id_valid_chars.size() - 1} {
}

BlankNode RandomIdGenerator::generate(storage::DynNodeStoragePtr node_storage) noexcept {
    std::array<char, generator_detail::generated_id_size> buf;

    {
        std::unique_lock lock{mutex_};
        std::ranges::generate(buf, [this]() { return this->next_char(); });
    }

    return BlankNode::make_unchecked(std::string_view{buf.data(), buf.size()}, node_storage);
}

char RandomIdGenerator::next_char() {
    return generator_detail::bnode_id_valid_chars[dist_(rng_)];
}

}  //namespace rdf4cpp::bnode_mngt
