#include "Variable.hpp"

#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/CharMatcher.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>

#include <uni_algo/all.h>

#include <cstring>

namespace rdf4cpp::query {

namespace detail_variable_inlining {
    inline constexpr size_t max_inlined_name_len = (storage::identifier::NodeID::width / (sizeof(char) * 8)) - 1; // -1 for anonymous tagging boolean

    struct InlinedRepr {
        char name[max_inlined_name_len]{};
        bool is_anonymous;

        [[nodiscard]] storage::view::VariableBackendView view() const {
            return storage::view::VariableBackendView{.name = std::string_view{name, strnlen(name, max_inlined_name_len)},
                                                      .is_anonymous =  is_anonymous};
        }

        std::strong_ordering operator<=>(InlinedRepr const &other) const noexcept {
            return view() <=> other.view();
        }
    };

    [[nodiscard]] inline storage::identifier::NodeBackendID try_into_inlined(std::string_view const name, bool const is_anonymous) noexcept {
        using namespace storage::identifier;

        if (name.size() > max_inlined_name_len) {
            return NodeBackendID{};
        }

        InlinedRepr inlined_data{};
        inlined_data.is_anonymous = is_anonymous;
        memcpy(&inlined_data.name, name.data(), name.size());
        // since inlined_data is initialized, there are implicitly nulls after the string if size is less than max

        auto const node_id = std::bit_cast<NodeID>(inlined_data);
        return NodeBackendID{node_id, RDFNodeType::Variable, true};
    }

    [[nodiscard]] constexpr InlinedRepr from_inlined(storage::identifier::NodeBackendID id) noexcept {
        RDF4CPP_ASSERT(id.is_inlined() && id.is_variable());
        return std::bit_cast<InlinedRepr>(id.node_id());
    }

} // namespace detail_variable_inlining

Variable::Variable() noexcept : Node{storage::identifier::NodeBackendHandle{}} {
}

Variable::Variable(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage)
    : Variable{make_unchecked((validate(name, anonymous), name), anonymous, node_storage)} {}

Variable::Variable(storage::identifier::NodeBackendHandle handle) noexcept : Node{handle} {}

Variable Variable::make_named(std::string_view name, storage::DynNodeStoragePtr node_storage) {
    return Variable{name, false, node_storage};
}

Variable Variable::make_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage) {
    return Variable{name, true, node_storage};
}

Variable Variable::make_unchecked(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage) {
    auto const node_backend_id = [&]() {
        if (auto const inlined_id = detail_variable_inlining::try_into_inlined(name, anonymous); !inlined_id.null()) {
            return inlined_id;
        }

        return node_storage.find_or_make_id(storage::view::VariableBackendView{.name = name, .is_anonymous = anonymous});
    }();

    return Variable{storage::identifier::NodeBackendHandle{node_backend_id, node_storage}};
}

Variable Variable::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    if (handle_.is_inlined()) {
        return Variable{storage::identifier::NodeBackendHandle{handle_.id(), node_storage}};
    }

    auto const node_id = node_storage.find_or_make_id(handle_.variable_backend());
    return Variable{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

Variable Variable::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    if (handle_.is_inlined()) {
        return Variable{storage::identifier::NodeBackendHandle{handle_.id(), node_storage}};
    }

    auto const node_id = node_storage.find_id(handle_.variable_backend());
    if (node_id.null()) {
        return Variable{};
    }

    return Variable{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

Variable Variable::find(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage) noexcept {
    auto const nid = [&]() {
        if (auto const inlined_id = detail_variable_inlining::try_into_inlined(name, anonymous); !inlined_id.null()) {
            return inlined_id;
        }

        return node_storage.find_id(storage::view::VariableBackendView{.name = name, .is_anonymous = anonymous});
    }();

    if (nid.null()) {
        return Variable{};
    }

    return Variable{storage::identifier::NodeBackendHandle{nid, node_storage}};
}
Variable Variable::find_named(std::string_view name, storage::DynNodeStoragePtr node_storage) noexcept {
    return find(name, false, node_storage);
}
Variable Variable::find_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage) noexcept {
    return find(name, true, node_storage);
}

bool Variable::is_anonymous() const {
    if (handle_.is_inlined()) {
        return detail_variable_inlining::from_inlined(handle_.id()).is_anonymous;
    }

    // TODO: encode is_anonymous into variable ID
    return handle_.variable_backend().is_anonymous;
}

CowString Variable::name() const {
    if (handle_.is_inlined()) {
        auto const inlined_repr = detail_variable_inlining::from_inlined(handle_.id());
        auto const name = inlined_repr.view().name;

        return CowString{CowString::owned, std::string{name}};
    }

    return CowString{CowString::borrowed, handle_.variable_backend().name};
}

FetchOrSerializeResult Variable::fetch_or_serialize_name(std::string_view &out, writer::BufWriterParts writer) const noexcept {
    if (handle_.is_inlined()) {
        auto const inlined_repr = detail_variable_inlining::from_inlined(handle_.id());
        if (!rdf4cpp::writer::write_str(inlined_repr.view().name, writer)) {
            return FetchOrSerializeResult::SerializationFailed;
        }

        return FetchOrSerializeResult::Serialized;
    }

    out = handle_.variable_backend().name;
    return FetchOrSerializeResult::Fetched;
}

bool Variable::serialize(writer::BufWriterParts const writer) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    auto const run_ser = [&writer](std::string_view name, bool is_anon) {
        if (is_anon) {
            RDF4CPP_DETAIL_TRY_WRITE_STR("_:");
        } else {
            RDF4CPP_DETAIL_TRY_WRITE_STR("?");
        }

        RDF4CPP_DETAIL_TRY_WRITE_STR(name);
        return true;
    };

    if (handle_.is_inlined()) {
        auto backend = detail_variable_inlining::from_inlined(handle_.id());
        auto const [name, is_anon] = std::move(backend).view();

        return run_ser(name, is_anon);
    } else {
        auto const backend = handle_.variable_backend();
        return run_ser(backend.name, backend.is_anonymous);
    }
}

Variable::operator std::string() const {
    return writer::StringWriter::oneshot([this](auto &w) noexcept {
        return this->serialize(w);
    });
}

std::ostream &operator<<(std::ostream &os, Variable const &variable) {
    writer::BufOStreamWriter w{os};
    variable.serialize(w);
    w.finalize();

    return os;
}

void Variable::validate(std::string_view n, bool anonymous) {
    if (anonymous) {
        return BlankNode::validate(n);
    }

    using namespace util::char_matcher_detail;
    static constexpr auto first_matcher = ASCIINumMatcher{} | PNCharsUMatcher;
    auto r = n | una::views::utf8;
    auto it = r.begin();
    if (it == r.end()) {
        throw InvalidNode("invalid blank node label (empty string)");
    }
    if (!first_matcher.match(*it)) {
        throw InvalidNode(std::format("invalid blank node label {}", n));
    }
    ++it;

    static constexpr auto matcher = ASCIINumMatcher{} | PNCharsUMatcher | PNChars_UnicodePartMatcher{};
    while (it != r.end()) {
        if (!matcher.match(*it)) {
            throw InvalidNode(std::format("invalid blank node label {}", n));
        }
        ++it;
    }
}

std::strong_ordering Variable::order(Variable const &other) const noexcept {
    if (is_inlined()) {
        auto const this_deinlined = detail_variable_inlining::from_inlined(handle_.id());
        if (other.is_inlined()) {
            auto const other_deinlined = detail_variable_inlining::from_inlined(other.handle_.id());
            return this_deinlined <=> other_deinlined;
        }

        return this_deinlined.view() <=> other.handle_.variable_backend();
    } else {
        if (other.is_inlined()) {
            auto const other_deinlined = detail_variable_inlining::from_inlined(other.handle_.id());
            return handle_.variable_backend() <=> other_deinlined.view();
        }

        return handle_.variable_backend() <=> other.handle_.variable_backend();
    }
}

bool Variable::order_eq(Variable const &other) const noexcept {
    return order(other) == std::strong_ordering::equal;
}
bool Variable::order_ne(Variable const &other) const noexcept {
    return !order_eq(other);
}

bool Variable::eq(Variable const &other) const noexcept {
    // for variables order_eq and eq are the same
    return order_eq(other);
}
bool Variable::ne(Variable const &other) const noexcept {
    return !eq(other);
}

}  // namespace rdf4cpp::query

auto std::formatter<rdf4cpp::query::Variable>::format(rdf4cpp::query::Variable n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
