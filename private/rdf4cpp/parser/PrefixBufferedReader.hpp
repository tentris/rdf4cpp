#ifndef RDF4CPP_PARSER_PRIVATE_PREFIXBUFFEREDREADER_HPP
#define RDF4CPP_PARSER_PRIVATE_PREFIXBUFFEREDREADER_HPP

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

namespace rdf4cpp::parser {

    /**
     * Wraps a C-like IO stream (void*, ReadFunc, ErrorFunc, EOFFunc) and
     * serves a buffered prefix first, then delegates to the underlying stream.
     *
     * This is used when we peek at the start of a stream for content sniffing
     * but need to replay those bytes for the actual parser.
     */
    struct PrefixBufferedReader {
        void *underlying_stream;
        ReadFunc underlying_read;
        ErrorFunc underlying_error;
        EOFFunc underlying_eof;

        std::vector<char> prefix_buf;
        size_t prefix_offset = 0;

        PrefixBufferedReader(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof, std::vector<char> prefix)
            : underlying_stream{stream},
              underlying_read{read},
              underlying_error{error},
              underlying_eof{eof},
              prefix_buf{std::move(prefix)} {
        }

        static size_t read_func(void *buffer, size_t elem_size, size_t count, void *voided_self) noexcept {
            auto *self = static_cast<PrefixBufferedReader *>(voided_self);
            auto *buf = static_cast<char *>(buffer);
            size_t total_bytes = elem_size * count;
            size_t bytes_read = 0;

            // serve from the prefix buffer first
            size_t const prefix_remaining = self->prefix_buf.size() - self->prefix_offset;
            if (prefix_remaining > 0) {
                size_t const from_prefix = std::min(total_bytes, prefix_remaining);
                std::memcpy(buf, self->prefix_buf.data() + self->prefix_offset, from_prefix);
                self->prefix_offset += from_prefix;
                bytes_read += from_prefix;
                total_bytes -= from_prefix;
                buf += from_prefix;
            }

            // delegate remaining to the underlying stream
            if (total_bytes > 0) {
                bytes_read += self->underlying_read(buf, 1, total_bytes, self->underlying_stream);
            }

            return bytes_read;
        }

        static int error_func(void *voided_self) noexcept {
            auto const *self = static_cast<PrefixBufferedReader *>(voided_self);
            return self->underlying_error(self->underlying_stream);
        }

        static int eof_func(void *voided_self) noexcept {
            auto const *self = static_cast<PrefixBufferedReader *>(voided_self);
            // not at eof if we still have buffered prefix data
            if (self->prefix_offset < self->prefix_buf.size()) {
                return 0;
            }
            return self->underlying_eof(self->underlying_stream);
        }
    };

}  // namespace rdf4cpp::parser

#endif  // RDF4CPP_PARSER_PRIVATE_PREFIXBUFFEREDREADER_HPP
