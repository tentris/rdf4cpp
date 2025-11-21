#ifndef RDF4CPP_PARSER_PRIVATE_IMPL_HPP
#define RDF4CPP_PARSER_PRIVATE_IMPL_HPP

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <serd/serd.h>

#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

namespace rdf4cpp::parser {

struct IStreamQuadIterator::ImplSerd final : Impl {
    using flags_type = IStreamQuadIterator::flags_type;
    using state_type = IStreamQuadIterator::state_type;
    using ok_type = IStreamQuadIterator::ok_type;
    using error_type = IStreamQuadIterator::error_type;

private:
    // workaround for gcc-14 bug, erroneously warns on unsing a lambda here
    // see https://github.com/NVIDIA/stdexec/issues/1143
    struct SerdReaderDtorLambda {
        void operator()(SerdReader* r) const {
            serd_reader_end_stream(r);
            serd_reader_free(r);
        }
    };
    std::unique_ptr<SerdReader, SerdReaderDtorLambda> reader;

    std::unique_ptr<state_type> state_owned = nullptr;
    state_type *state;

    std::deque<Quad> quad_buffer;
    std::optional<ParsingError> last_error;
    bool last_error_requires_skip = false;
    bool end_flag = false;

    flags_type flags;

    static std::string_view node_into_string_view(SerdNode const *node) noexcept;
    static ParsingError::Type parsing_error_type_from_serd(SerdStatus st) noexcept;

    nonstd::expected<Node, SerdStatus> get_bnode(std::string &&graph_str, SerdNode const *node) noexcept;
    nonstd::expected<IRI, SerdStatus> get_iri(SerdNode const *node) noexcept;
    nonstd::expected<IRI, SerdStatus> get_prefixed_iri(SerdNode const *node) noexcept;
    nonstd::expected<Literal, SerdStatus> get_literal(SerdNode const *literal, SerdNode const *datatype, SerdNode const *lang) noexcept;
    SerdStatus inspect_node(Node const &node) noexcept;

    static SerdStatus on_error(void *voided_self, SerdError const *error) noexcept;
    static SerdStatus on_base(void *voided_self, SerdNode const *uri) noexcept;
    static SerdStatus on_prefix(void *voided_self, SerdNode const *name, SerdNode const *uri) noexcept;
    static SerdStatus on_stmt(void *voided_self, SerdStatementFlags, SerdNode const *graph, SerdNode const *subj, SerdNode const *pred, SerdNode const *obj, SerdNode const *obj_datatype, SerdNode const *obj_lang) noexcept;

    static constexpr SerdSyntax extract_syntax_from_flags(ParsingFlags flags) noexcept {
        switch (flags.get_syntax()) {
            case ParsingFlag::TriG:
                return SerdSyntax::SERD_TRIG;
            case ParsingFlag::NTriples:
                return SerdSyntax::SERD_NTRIPLES;
            case ParsingFlag::NQuads:
                return SerdSyntax::SERD_NQUADS;
            default:
                return SerdSyntax::SERD_TURTLE;
        }
    }

public:
    ImplSerd(void *stream,
         ReadFunc read,
         ErrorFunc,
         flags_type flags,
         state_type *state) noexcept;

    ~ImplSerd() override;

    /**
     * Tries to extract the next element from the serd backend.
     * Will try to skip over errors so that the next call might be able to return a value.
     *
     * @note Call until std::nullopt is returned
     * @return
     *      std::nullopt: if there is no next element (eof)
     *      expected Quad: if there was a next element and it could be parsed
     *      unexpected ParsingError: if there was a next element but it could not be parsed
     */
    [[nodiscard]] std::optional<nonstd::expected<ok_type, error_type>> next() override;

    [[nodiscard]] uint64_t current_line() const noexcept override;
    [[nodiscard]] uint64_t current_column() const noexcept override;
};

}  // namespace rdf4cpp::parser

#endif  // RDF4CPP_PARSER_PRIVATE_IMPL_HPP
