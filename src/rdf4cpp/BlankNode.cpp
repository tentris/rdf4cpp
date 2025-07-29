#include "BlankNode.hpp"
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/CharMatcher.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/Assert.hpp>
#include <uni_algo/all.h>

#include <cstring>

namespace rdf4cpp {

namespace detail_bnode_inlining {
    inline constexpr size_t max_inlined_name_len = storage::identifier::NodeID::width / (sizeof(char) * 8);

    struct InlinedRepr {
        char identifier[max_inlined_name_len]{};

        [[nodiscard]] storage::view::BNodeBackendView view() const {
            return storage::view::BNodeBackendView{.identifier = std::string_view{identifier, strnlen(identifier, max_inlined_name_len)}};
        }

        std::strong_ordering operator<=>(InlinedRepr const &other) const noexcept {
            return view() <=> other.view();
        }
    };

    [[nodiscard]] inline storage::identifier::NodeBackendID try_into_inlined(std::string_view const identifier) noexcept {
        using namespace storage::identifier;

        if (identifier.size() > max_inlined_name_len) {
            return NodeBackendID{};
        }

        InlinedRepr inlined_data{};
        memcpy(&inlined_data.identifier, identifier.data(), identifier.size());
        // since inlined_data is initialized, there are implicitly nulls after the string if size is less than max

        auto const node_id = std::bit_cast<NodeID>(inlined_data);
        return NodeBackendID{node_id, RDFNodeType::BNode, true};
    }

    [[nodiscard]] constexpr InlinedRepr from_inlined(storage::identifier::NodeBackendID id) noexcept {
        RDF4CPP_ASSERT(id.is_inlined() && id.is_blank_node());
        return std::bit_cast<InlinedRepr>(id.node_id());
    }

} // namespace detail_bnode_inlining

BlankNode::BlankNode() noexcept : Node{storage::identifier::NodeBackendHandle{}} {
}

BlankNode::BlankNode(std::string_view identifier, storage::DynNodeStoragePtr node_storage)
    : BlankNode{make_unchecked((validate(identifier), identifier), node_storage)} {
}

BlankNode::BlankNode(storage::identifier::NodeBackendHandle handle) noexcept : Node{handle} {
}

BlankNode BlankNode::make_null() noexcept {
    return BlankNode{};
}

BlankNode BlankNode::make(std::string_view identifier, storage::DynNodeStoragePtr node_storage) {
    return BlankNode{identifier, node_storage};
}

BlankNode BlankNode::make_unchecked(std::string_view identifier, storage::DynNodeStoragePtr node_storage) {
    auto const id = [&]() {
        if (auto const inlined = detail_bnode_inlining::try_into_inlined(identifier); !inlined.null()) {
            return inlined;
        }

        return node_storage.find_or_make_id(storage::view::BNodeBackendView{.identifier = identifier});
    }();

    return BlankNode{storage::identifier::NodeBackendHandle{id, node_storage}};
}

BlankNode BlankNode::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    if (handle_.is_inlined()) {
        return BlankNode{storage::identifier::NodeBackendHandle{handle_.id(), node_storage}};
    }

    auto const node_id = node_storage.find_or_make_id(handle_.bnode_backend());
    return BlankNode{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

BlankNode BlankNode::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    if (handle_.is_inlined()) {
        return BlankNode{storage::identifier::NodeBackendHandle{handle_.id(), node_storage}};
    }

    auto const node_id = node_storage.find_id(handle_.bnode_backend());
    if (node_id.null()) {
        return BlankNode{};
    }

    return BlankNode{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

BlankNode BlankNode::find(std::string_view identifier, storage::DynNodeStoragePtr node_storage) noexcept {
    auto const nid = [&]() {
        if (auto const inlined = detail_bnode_inlining::try_into_inlined(identifier); !inlined.null()) {
            return inlined;
        }

        return node_storage.find_id(storage::view::BNodeBackendView{.identifier = identifier});
    }();

    if (nid.null()) {
        return BlankNode{};
    }

    return BlankNode{storage::identifier::NodeBackendHandle{nid, node_storage}};
}

CowString BlankNode::identifier() const noexcept {
    if (handle_.is_inlined()) {
        auto const inlined = detail_bnode_inlining::from_inlined(handle_.id());
        auto const identifier = inlined.view().identifier;

        return CowString{CowString::owned, std::string{identifier}};
    }

    return CowString{CowString::borrowed, handle_.bnode_backend().identifier};
}

FetchOrSerializeResult BlankNode::fetch_or_serialize_identifier(std::string_view &out, writer::BufWriterParts writer) const noexcept {
    if (handle_.is_inlined()) {
        auto const inlined = detail_bnode_inlining::from_inlined(handle_.id());
        if (!rdf4cpp::writer::write_str(inlined.view().identifier, writer)) [[unlikely]] {
            return FetchOrSerializeResult::SerializationFailed;
        }

        return FetchOrSerializeResult::Serialized;
    }

    out = handle_.bnode_backend().identifier;
    return FetchOrSerializeResult::Fetched;
}

bool BlankNode::serialize(writer::BufWriterParts const writer) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    auto const ident = identifier();

    RDF4CPP_DETAIL_TRY_WRITE_STR("_:");
    RDF4CPP_DETAIL_TRY_WRITE_STR(ident.view());
    return true;
}

BlankNode::operator std::string() const noexcept {
    return writer::StringWriter::oneshot([this](auto &w) noexcept {
        return this->serialize(w);
    });
}

std::ostream &operator<<(std::ostream &os, BlankNode const &bnode) {
    writer::BufOStreamWriter w{os};
    bnode.serialize(w);
    w.finalize();

    return os;
}

void BlankNode::validate(std::string_view v) {
    using namespace util::char_matcher_detail;
    static constexpr auto first_matcher = ASCIINumMatcher{} | PNCharsUMatcher;
    auto r = v | una::views::utf8;
    auto it = r.begin();
    if (it == r.end()) {
        throw InvalidNode("invalid blank node label (empty string)");
    }
    if (!first_matcher.match(*it)) {
        throw InvalidNode(std::format("invalid blank node label {}", v));
    }
    auto lastchar = *it;
    ++it;
    static constexpr auto pn_matcher = PNCharsMatcher | ASCIIPatternMatcher{"."};
    while (it != r.end()) {
        if (!pn_matcher.match(*it))
        {
            throw InvalidNode(std::format("invalid blank node label {}", v));
        }
        lastchar = *it;
        ++it;
    }
    if (lastchar == '.') {
        throw InvalidNode(std::format("invalid blank node label {}", v));
    }
}

std::strong_ordering BlankNode::order(BlankNode const &other) const noexcept {
    if (is_inlined()) {
        auto const this_delined = detail_bnode_inlining::from_inlined(handle_.id());
        if (other.is_inlined()) {
            auto const other_deinlined = detail_bnode_inlining::from_inlined(other.handle_.id());
            return this_delined <=> other_deinlined;
        }

        return this_delined.view() <=> other.handle_.bnode_backend();
    } else {
        if (other.is_inlined()) {
            auto const other_deinlined = detail_bnode_inlining::from_inlined(other.handle_.id());
            return handle_.bnode_backend() <=> other_deinlined.view();
        }

        return handle_.bnode_backend() <=> other.handle_.bnode_backend();
    }
}

bool BlankNode::order_eq(BlankNode const &other) const noexcept {
    return order(other) == std::strong_ordering::equivalent;
}

bool BlankNode::order_ne(BlankNode const &other) const noexcept {
    return !order_eq(other);
}

bool BlankNode::eq(BlankNode const &other) const noexcept {
    // there is no difference between order_eq and eq for blank nodes
    return order_eq(other);
}

bool BlankNode::ne(BlankNode const &other) const noexcept {
    return !eq(other);
}

inline namespace shorthands {

BlankNode operator""_bnode(char const *str, size_t len) {
    return BlankNode{std::string_view{str, len}};
}

}  // namespace literals
}  // namespace rdf4cpp

auto std::formatter<rdf4cpp::BlankNode>::format(rdf4cpp::BlankNode n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
