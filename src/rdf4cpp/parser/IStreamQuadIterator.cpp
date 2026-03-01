#include "IStreamQuadIterator.hpp"

#include <rdf4cpp/parser/IStreamQuadIteratorSerdImpl.hpp>
#include <rdf4cpp/parser/PrefixBufferedReader.hpp>
#include <rdf4cpp/parser/XMLParser.hpp>

#include <cstdio>
#include <stdexcept>
#include <vector>

#if __has_include(<fcntl.h>)
#include <fcntl.h>
#endif //__has_include

namespace rdf4cpp::parser {

static constexpr size_t peek_size = 4096;

/**
 * Adaptor function so that serd can read from std::istreams.
 * Matches the interface of SerdSource/fread
 *
 * @param buf pointer to buffer to be written to
 * @param elem_size sizeof each element being written, guaranteed to always be 1
 * @param count number of elements to write
 * @param voided_self pointer to std::istream cast to void *
 */
static size_t istream_read(void *buf, [[maybe_unused]] size_t elem_size, size_t count, void *voided_self) noexcept {
    RDF4CPP_ASSERT(elem_size == 1);

    auto *self = static_cast<std::istream *>(voided_self);
    self->read(static_cast<char *>(buf), static_cast<std::streamsize>(count));
    return self->gcount();
}

/**
 * Adaptor function for serd to check if an std::istream is ok
 * Matches the interface of SerdStreamErrorFunc
 *
 * @param voided_self pointer to std::istream cast to void *
 * @return whether the given istream encountered an error (cast to int)
 */
static int istream_error(void *voided_self) noexcept {
    auto *self = static_cast<std::istream *>(voided_self);
    return static_cast<int>(self->fail() && !self->eof());
}

/**
 * Adaptor function for serd to check if an std::istream is at the end of file
 *
 * @param voided_self pointer to std::istream cast to void *
 * @return whether the given istream encountered an error (cast to int)
 */
static int istream_eof(void *voided_self) noexcept {
    auto *self = static_cast<std::istream *>(voided_self);
    return static_cast<int>(self->eof());
}

static void throw_if_unsupported(ParsingFlag syntax) {
    if (syntax == ParsingFlag::OwlXml) {
        throw std::runtime_error("OWL/XML format is not supported. Please convert to RDF/XML or Turtle.");
    }
    if (syntax == ParsingFlag::JsonLd) {
        throw std::runtime_error("JSON-LD format is not supported.");
    }
}

static ParsingFlag resolve_auto_syntax(FormatGuess guess) {
    if (guess.is_known()) {
        return guess.syntax;
    }
    // fallback to Turtle when we can't determine the format
    return ParsingFlag::Turtle;
}

IStreamQuadIterator::IStreamQuadIterator(void *stream,
                                         ReadFunc read,
                                         ErrorFunc error,
                                         EOFFunc eof,
                                         flags_type flags,
                                         state_type *state) {
    auto make_impl = [](void *s, ReadFunc r, ErrorFunc e, EOFFunc ef,
                        flags_type f, state_type *st) -> std::unique_ptr<Impl> {
        if (f.get_syntax() == ParsingFlag::RdfXml) {
            return std::make_unique<ImplXML>(s, r, e, ef, st);
        }
        return std::make_unique<ImplSerd>(s, r, e, f, st);
    };

    if (flags.get_syntax() == ParsingFlag::Auto) {
        // Peek bytes for content sniffing
        std::vector<char> buf(peek_size);
        size_t const bytes_read = read(buf.data(), 1, peek_size, stream);
        buf.resize(bytes_read);

        std::string_view const prefix{buf.data(), buf.size()};
        auto const guess = guess_format_from_content(prefix);
        auto const resolved = resolve_auto_syntax(guess);
        throw_if_unsupported(resolved);

        detected_format_ = guess;
        auto const resolved_flags = flags.with_syntax(resolved);

        // Create a PrefixBufferedReader to replay peeked bytes
        buffered_reader_ = std::make_unique<PrefixBufferedReader>(stream, read, error, eof, std::move(buf));
        impl = make_impl(buffered_reader_.get(),
                         &PrefixBufferedReader::read_func,
                         &PrefixBufferedReader::error_func,
                         &PrefixBufferedReader::eof_func,
                         resolved_flags, state);
    } else {
        throw_if_unsupported(flags.get_syntax());
        detected_format_ = FormatGuess{flags.get_syntax(), GuessConfidence::Certain};
        impl = make_impl(stream, read, error, eof, flags, state);
    }
    cur = impl->next();
}

IStreamQuadIterator::IStreamQuadIterator(std::istream &istream,
                                         flags_type flags,
                                         state_type *state)
    : IStreamQuadIterator{&istream, &istream_read, &istream_error, &istream_eof, flags, state} {
}

IStreamQuadIterator::IStreamQuadIterator(IStreamQuadIterator &&other) noexcept = default;
IStreamQuadIterator &IStreamQuadIterator::operator=(IStreamQuadIterator &&) noexcept = default;

IStreamQuadIterator::~IStreamQuadIterator() noexcept = default;

typename IStreamQuadIterator::reference IStreamQuadIterator::operator*() const noexcept {
    return *cur;
}

typename IStreamQuadIterator::pointer IStreamQuadIterator::operator->() const noexcept {
    return &*cur;
}

IStreamQuadIterator &IStreamQuadIterator::operator++() {
    cur = impl->next();
    return *this;
}

uint64_t IStreamQuadIterator::current_line() const noexcept {
    return impl->current_line();
}

uint64_t IStreamQuadIterator::current_column() const noexcept {
    return impl->current_column();
}

FormatGuess IStreamQuadIterator::detected_format() const noexcept {
    return detected_format_;
}

bool IStreamQuadIterator::operator==(std::default_sentinel_t) const noexcept {
    return !cur.has_value();
}

bool IStreamQuadIterator::operator!=(std::default_sentinel_t) const noexcept {
    return cur.has_value();
}

FILE *fopen_fastseq(char const *path, char const *mode) noexcept {
    // inspired by <serd/system.c> (serd_fopen)

    FILE *fd = fopen(path, mode);
    if (fd == nullptr) {
        return fd;
    }

#if __has_include(<fcntl.h>) && _POSIX_C_SOURCE >= 200112L
    (void) posix_fadvise(fileno(fd), 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE | POSIX_FADV_WILLNEED);
#endif

    return fd;
}

}  // namespace rdf4cpp::parser
