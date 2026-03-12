#include "IStreamQuadIterator.hpp"

#include <rdf4cpp/parser/IStreamQuadIteratorSerdImpl.hpp>
#include <rdf4cpp/parser/XMLParser.hpp>
#include <rdf4cpp/parser/JsonLdParser.hpp>

#include <cstdio>

#if __has_include(<fcntl.h>)
#include <fcntl.h>
#endif //__has_include

namespace rdf4cpp::parser {

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

std::unique_ptr<IStreamQuadIterator::Impl> IStreamQuadIterator::make_impl(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof, flags_type flags, state_type *state) {
    auto syntax = flags.get_syntax();
    if (syntax == ParsingFlag::RdfXml) {
        return std::make_unique<ImplXML>(stream, read, error, eof, state);
    }
    else if (syntax == ParsingFlag::JsonLd) {
        return std::make_unique<ImplJsonLd>(stream, read, error, eof, state);
    }
    else {
        return std::make_unique<ImplSerd>(stream, read, error, flags, state);
    }
}
IStreamQuadIterator::IStreamQuadIterator(void *stream,
                                         ReadFunc read,
                                         ErrorFunc error,
                                         EOFFunc eof,
                                         flags_type flags,
                                         state_type *state)
    : impl{make_impl(stream, read, error, eof, flags, state)},
      cur{impl->next()} {
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
