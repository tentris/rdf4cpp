#include "IStreamQuadIteratorSerdImpl.hpp"

#include <cassert>
#include <cstddef>

#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::parser {

std::string_view IStreamQuadIterator::ImplSerd::node_into_string_view(SerdNode const *node) noexcept {
    return std::string_view{reinterpret_cast<char const *>(node->buf), node->n_bytes};
}

ParsingError::Type IStreamQuadIterator::ImplSerd::parsing_error_type_from_serd(SerdStatus const st) noexcept {
    switch (st) {
        case SERD_ERR_BAD_SYNTAX:
            return ParsingError::Type::BadSyntax;
        case SERD_ERR_BAD_CURIE:
            return ParsingError::Type::BadCurie;
        case SERD_FAILURE:
            return ParsingError::Type::EofReached;
        case SERD_ERR_ID_CLASH:
            return ParsingError::Type::BadBlankNode;
        default:
            return ParsingError::Type::Internal;
    }
}

nonstd::expected<Node, SerdStatus> IStreamQuadIterator::ImplSerd::get_bnode(std::string &&graph_str, SerdNode const *node) noexcept {
    auto const node_str = node_into_string_view(node);

    if (this->flags.contains(ParsingFlag::NoParseBlankNode)) {
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = "Encountered blank node while parsing. hint: blank nodes are not allowed in the current document. note: position may not be accurate and instead point to the end of the line."};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    }

    try {
        if (this->state->blank_node_scope_manager == nullptr || this->flags.contains(ParsingFlag::KeepBlankNodeIds)) {
            return BlankNode{node_str, this->state->node_storage};
        }

        return this->state->blank_node_scope_manager.scope(graph_str).get_or_generate_node(node_str, this->state->node_storage);
    } catch (InvalidNode const &e) {
        // NOTE: line, col not entirely accurate as this function is called after a triple was parsed
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadBlankNode,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = std::string{e.what()} + ". note: position may not be accurate and instead point to the end of the triple."};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    } catch (...) {
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadBlankNode,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = "Unknown internal error. note: position may not be accurate and instead point to the end of the triple."};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    }
}

nonstd::expected<IRI, SerdStatus> IStreamQuadIterator::ImplSerd::get_iri(SerdNode const *node) noexcept {
    auto const iri = [this, node]() noexcept {
        auto const s = node_into_string_view(node);

        if (flags.syntax_allows_prefixes()) {
            return this->state->iri_factory.from_maybe_relative(s, this->state->node_storage);
        } else {
            return this->state->iri_factory.create_and_validate(s, this->state->node_storage);
        }
    }();

    if (!iri.has_value()) {
        IRIFactoryError err = iri.error();
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadIri,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = std::format("invalid iri. {}. note: position may not be accurate and instead point to the end of the triple.", err)};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    }

    return *iri;
}

nonstd::expected<IRI, SerdStatus> IStreamQuadIterator::ImplSerd::get_prefixed_iri(SerdNode const *node) noexcept {
    if (!flags.syntax_allows_prefixes()) [[unlikely]] {
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = "Encountered prefix while parsing. hint: prefixes are not allowed in the current document. note: position may not be accurate and instead point to the end of the line."};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    }

    auto const uri_node_view = node_into_string_view(node);

    auto const sep_pos = uri_node_view.find(':');
    if (sep_pos == std::string_view::npos) {
        return nonstd::make_unexpected(SERD_ERR_BAD_CURIE);
    }

    auto const prefix = uri_node_view.substr(0, sep_pos);
    auto const suffix = uri_node_view.substr(sep_pos + 1);

    auto iri = state->iri_factory.from_prefix(prefix, suffix, state->node_storage);
    if (!iri.has_value()) {
        IRIFactoryError err = iri.error();
        if (err == IRIFactoryError::UnknownPrefix) {
            // NOTE: line, col not entirely accurate as this function is called after a triple was parsed
            this->last_error = ParsingError{.error_type = ParsingError::Type::BadCurie,
                                            .line = serd_reader_get_current_line(this->reader.get()),
                                            .col = serd_reader_get_current_col(this->reader.get()),
                                            .message = "unknown prefix. note: position may not be accurate and instead point to the end of the triple."};

            return nonstd::make_unexpected(SERD_ERR_BAD_CURIE);
        } else {
            this->last_error = ParsingError{.error_type = ParsingError::Type::BadIri,
                                            .line = serd_reader_get_current_line(this->reader.get()),
                                            .col = serd_reader_get_current_col(this->reader.get()),
                                            .message = std::format("unable to expand curie into valid iri. {}. note: position may not be accurate and instead point to the end of the triple.", err)};

            return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }

    return *iri;
}

nonstd::expected<Literal, SerdStatus> IStreamQuadIterator::ImplSerd::get_literal(SerdNode const *literal, SerdNode const *datatype, SerdNode const *lang) noexcept {
    auto const literal_value = node_into_string_view(literal);

    auto const datatype_iri = [&]() -> std::optional<nonstd::expected<IRI, SerdStatus>> {
        if (datatype != nullptr) {
            switch (datatype->type) {
                case SerdType::SERD_CURIE:
                    return this->get_prefixed_iri(datatype);
                case SerdType::SERD_URI:
                    return this->get_iri(datatype);
                default:
                    RDF4CPP_UNREACHABLE;
            }
        } else {
            return std::nullopt;
        }
    }();

    try {
        if (datatype_iri.has_value()) {
            if (!datatype_iri->has_value()) {
                return nonstd::make_unexpected(datatype_iri->error());
            }

            return Literal::make_typed(literal_value, datatype_iri->value(), this->state->node_storage);
        } else if (lang != nullptr) {
            return Literal::make_lang_tagged(literal_value, node_into_string_view(lang), this->state->node_storage);
        } else {
            return Literal::make_simple(literal_value, this->state->node_storage);
        }
    } catch (InvalidNode const &e) {
        // NOTE: line, col not entirely accurate as this function is called after a triple was parsed
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadLiteral,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = std::string{e.what()} + ". note: position may not be accurate and instead point to the end of the triple."};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    } catch (...) {
        this->last_error = ParsingError{.error_type = ParsingError::Type::BadLiteral,
                                        .line = serd_reader_get_current_line(this->reader.get()),
                                        .col = serd_reader_get_current_col(this->reader.get()),
                                        .message = "Unknown internal error. note: position may not be accurate and instead point to the end of the triple."};

        return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
    }
}

/**
 * Calculate the required buffer size for a vsnprintf call for formatting a SerdError
 * @param error the error to format
 * @return buffer size
 */
[[nodiscard]] SerdStatus calc_required_buffer_size(SerdError const *error, size_t &out) noexcept {
    // we need to copy the va_list because otherwise our call to vsnprintf would consume it which leads to undefined
    // behaviour when we try to actually vsnprintf
    va_list copy;
    va_copy(copy, *error->args);
    int const ret = vsnprintf(nullptr, 0, error->fmt, copy);
    va_end(copy);

    if (ret < 0) [[unlikely]] {
        // vsnprintf failed
        return SERD_FAILURE;
    }

    out = static_cast<size_t>(ret) + 1; // +1 for null terminator
    return SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::ImplSerd::on_error(void *voided_self, SerdError const *error) noexcept {
    auto *self = static_cast<ImplSerd *>(voided_self);

    size_t buf_size;
    SerdStatus const st = calc_required_buffer_size(error, buf_size);
    if (st != SERD_SUCCESS) [[unlikely]] {
        return st;
    }

    std::string message;
    message.resize(buf_size);

    int const bytes_written = vsnprintf(message.data(), message.size(), error->fmt, *error->args);
    if (bytes_written < 0) [[unlikely]] {
        // vsnprintf failed
        return SERD_FAILURE;
    }

    // drop null-terminator from vsnprintf and newline from serd
    message.resize(static_cast<size_t>(bytes_written));
    if (message.ends_with('\n')) {
        message.pop_back();
    }

    self->last_error = ParsingError{.error_type = parsing_error_type_from_serd(error->status),
                                    .line = error->line,
                                    .col = error->col,
                                    .message = std::move(message)};
    self->last_error_requires_skip = true;

    return SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::ImplSerd::on_base(void *voided_self, const SerdNode *uri) noexcept {
    auto *self = static_cast<ImplSerd *>(voided_self);

    if (self->flags.contains(ParsingFlag::NoParsePrefix)) {
        self->last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                        .line = serd_reader_get_current_line(self->reader.get()),
                                        .col = serd_reader_get_current_col(self->reader.get()),
                                        .message = "Encountered base while parsing. hint: bases are not allowed in the current document. note: position may not be accurate and instead point to the end of the line."};
    } else if (auto e = self->state->iri_factory.set_base(node_into_string_view(uri)); e != IRIFactoryError::Ok) {
        self->last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                        .line = serd_reader_get_current_line(self->reader.get()),
                                        .col = serd_reader_get_current_col(self->reader.get()),
                                        .message = std::format("Error setting base: {}. note: position may not be accurate and instead point to the end of the line.", e)};
    }

    return SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::ImplSerd::on_prefix(void *voided_self, SerdNode const *name, SerdNode const *uri) noexcept {
    auto *self = static_cast<ImplSerd *>(voided_self);

    if (self->flags.contains(ParsingFlag::NoParsePrefix)) {
        self->last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                        .line = serd_reader_get_current_line(self->reader.get()),
                                        .col = serd_reader_get_current_col(self->reader.get()),
                                        .message = "Encountered prefix while parsing. hint: prefixes are not allowed in the current document. note: position may not be accurate and instead point to the end of the line."};
    } else {
        if (self->state->iri_factory.assign_prefix(node_into_string_view(name), node_into_string_view(uri)) != IRIFactoryError::Ok) {
            self->last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                            .line = serd_reader_get_current_line(self->reader.get()),
                                            .col = serd_reader_get_current_col(self->reader.get()),
                                            .message = std::format("Invalid prefix: {}. note: position may not be accurate and instead point to the end of the line.", node_into_string_view(name))};
        }
    }

    return SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::ImplSerd::inspect_node(Node const &node) noexcept {
    try {
        state->inspect_node_func(node);
        return SERD_SUCCESS;
    } catch (std::exception const &e) {
        // skip
        last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                  .line = serd_reader_get_current_line(reader.get()),
                                  .col = serd_reader_get_current_col(reader.get()),
                                  .message = std::format("Triple explicitly skipped by inspect function: {}", e.what())};
    } catch (...) {
        last_error = ParsingError{.error_type = ParsingError::Type::BadSyntax,
                                  .line = serd_reader_get_current_line(reader.get()),
                                  .col = serd_reader_get_current_col(reader.get()),
                                  .message = "Triple explicitly skipped by inspect function"};
    }

    return SERD_FAILURE;
}

SerdStatus IStreamQuadIterator::ImplSerd::on_stmt(void *voided_self,
                                              SerdStatementFlags,
                                              SerdNode const *graph,
                                              SerdNode const *subj,
                                              SerdNode const *pred,
                                              SerdNode const *obj,
                                              SerdNode const *obj_datatype,
                                              SerdNode const *obj_lang) noexcept {

    auto *self = static_cast<ImplSerd *>(voided_self);

    auto const graph_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        if (graph != nullptr) {
            switch (graph->type) {
                case SERD_CURIE:
                    return self->get_prefixed_iri(graph);
                case SERD_URI:
                    return self->get_iri(graph);
                case SERD_BLANK:
                    return self->get_bnode("", graph); // TODO which scope?
                default:
                    return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
            }
        } else {
            return IRI::default_graph(self->state->node_storage);
        }
    }();

    if (!graph_node.has_value()) {
        return SERD_SUCCESS;
    }

    if (self->inspect_node(*graph_node) != SERD_SUCCESS) {
        return SERD_SUCCESS;
    }

    auto const subj_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        switch (subj->type) {
            case SERD_CURIE:
                return self->get_prefixed_iri(subj);
            case SERD_URI:
                return self->get_iri(subj);
            case SERD_BLANK:
                return self->get_bnode(std::string{*graph_node}, subj);
            default:
                return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }();

    if (!subj_node.has_value()) {
        return SERD_SUCCESS;
    }

    if (self->inspect_node(*subj_node) != SERD_SUCCESS) {
        return SERD_SUCCESS;
    }

    auto const pred_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        switch (pred->type) {
            case SERD_CURIE:
                return self->get_prefixed_iri(pred);
            case SERD_URI:
                return self->get_iri(pred);
            default:
                return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }();

    if (!pred_node.has_value()) {
        return SERD_SUCCESS;
    }

    if (self->inspect_node(*pred_node) != SERD_SUCCESS) {
        return SERD_SUCCESS;
    }

    auto const obj_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        switch (obj->type) {
            case SERD_CURIE:
                return self->get_prefixed_iri(obj);
            case SERD_LITERAL:
                return self->get_literal(obj, obj_datatype, obj_lang);
            case SERD_BLANK:
                return self->get_bnode(std::string{*graph_node}, obj);
            case SERD_URI:
                return self->get_iri(obj);
            default:
                return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }();

    if (!obj_node.has_value()) {
        return SERD_SUCCESS;
    }

    if (self->inspect_node(*obj_node) != SERD_SUCCESS) {
        return SERD_SUCCESS;
    }

    self->quad_buffer.emplace_back(*graph_node, *subj_node, *pred_node, *obj_node);
    return SERD_SUCCESS;
}

IStreamQuadIterator::ImplSerd::ImplSerd(void *stream,
                                ReadFunc read,
                                ErrorFunc error,
                                flags_type flags,
                                state_type *initial_state) noexcept
    : reader{serd_reader_new(extract_syntax_from_flags(flags), this, nullptr, &ImplSerd::on_base, &ImplSerd::on_prefix, &ImplSerd::on_stmt, nullptr)},
      state{initial_state},
      state_is_owned{false},
      flags{flags} {
    if (this->state == nullptr) {
        this->state = new state_type{};
        this->state_is_owned = true;
    }

    serd_reader_set_strict(this->reader.get(), !flags.contains(ParsingFlag::Lax));
    serd_reader_set_error_sink(this->reader.get(), &ImplSerd::on_error, this);
    serd_reader_start_source_stream(this->reader.get(), read, error, stream, nullptr, 4096);
}
IStreamQuadIterator::ImplSerd::~ImplSerd() {
    if (this->state_is_owned) {
        delete this->state;
    }
}

std::optional<nonstd::expected<IStreamQuadIterator::ok_type, IStreamQuadIterator::error_type>> IStreamQuadIterator::ImplSerd::next() {
    while (this->quad_buffer.empty()) {
        if (this->last_error.has_value()) {
            // handle error from last time
            if (this->last_error_requires_skip) {
                this->last_error_requires_skip = false;
                if (serd_reader_skip_until_byte(this->reader.get(), '\n') != SERD_SUCCESS) {
                    // EOF reached
                    this->end_flag = true;
                }
            }
            return nonstd::make_unexpected(*std::exchange(this->last_error, std::nullopt));
        } else if (this->end_flag) {
            return std::nullopt;
        }

        SerdStatus const st = serd_reader_read_chunk(this->reader.get());

        if (st == SERD_SUCCESS) {
            // was able to parse something
            // not sure if triple or something else, continue loop
            continue;
        }

        if (!this->last_error.has_value()) {
            if (st == SERD_FAILURE) {
                // no error and SERD_FAILURE means EOF
                this->end_flag = true;
            } else {
                // got some other specific error => not eof
                // but we don't really know what because the error handler was not called
                this->last_error = ParsingError{.error_type = parsing_error_type_from_serd(st),
                                                .line = serd_reader_get_current_line(this->reader.get()),
                                                .col = serd_reader_get_current_col(this->reader.get()),
                                                .message = "Unknown error"};
                this->last_error_requires_skip = true;
            }
        }
    }

    auto ret = this->quad_buffer.front();
    this->quad_buffer.pop_front();
    return ret;
}

uint64_t IStreamQuadIterator::ImplSerd::current_line() const noexcept {
    return serd_reader_get_current_line(this->reader.get());
}

uint64_t IStreamQuadIterator::ImplSerd::current_column() const noexcept {
    return serd_reader_get_current_col(this->reader.get());
}

}  // namespace rdf4cpp::parser
