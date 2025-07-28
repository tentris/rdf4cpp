#include <rdf4cpp/bnode_mngt/reference_backends/generator/IncreasingIdGenerator.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <limits>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::bnode_mngt {

namespace generator_detail {
static constexpr size_t max_generated_id_size = std::numeric_limits<size_t>::digits10;
} //namespace generator_detail

IncreasingIdGenerator::IncreasingIdGenerator(std::string prefix, size_t const initial_value) noexcept : prefix_{std::move(prefix)},
                                                                                                        counter_{initial_value} {
    BlankNode::validate(prefix_); // throws if invalid
}

BlankNode IncreasingIdGenerator::generate(storage::DynNodeStoragePtr node_storage) noexcept {
    static thread_local std::string buf;

    buf.clear();
    buf.resize(prefix_.size() + generator_detail::max_generated_id_size);

    auto write_it = std::copy(prefix_.begin(), prefix_.end(), buf.data());
    std::to_chars_result const res = std::to_chars(write_it, write_it + generator_detail::max_generated_id_size, counter_.fetch_add(1, std::memory_order_relaxed));
    RDF4CPP_ASSERT(res.ec == std::errc{});

    // checked in constructor, can use make_unchecked
    return BlankNode::make_unchecked(std::string_view{buf.data(), static_cast<size_t>(res.ptr - buf.data())}, node_storage);
}

}  //namespace rdf4cpp::bnode_mngt
