#ifndef RDF4CPP_PARSER_ISTREAMQUADITERATOR_HPP
#define RDF4CPP_PARSER_ISTREAMQUADITERATOR_HPP

#include <iterator>
#include <memory>

#include <rdf4cpp/Expected.hpp>

#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/parser/internal/IStreamQuadIteratorSerdImpl.hpp>
#include <rdf4cpp/parser/ParsingError.hpp>
#include <rdf4cpp/parser/ParsingFlags.hpp>
#include <rdf4cpp/parser/ParsingState.hpp>

namespace rdf4cpp::parser {

/**
 * Similar to std::istream_iterator<>.
 * Parses the given istream and tries to extract Quads given in TURTLE format.
 *
 * @note the iterator _starts on_ the first Quad
 * @note An exhausted iterator becomes the end-of-stream iterator.
 * @warning The value pointed to by an end-of-stream iterator is undefined
 *
 * @example
 * @code
 * std::ifstream ifs{"triples.nt"};
 *
 * for (auto qit = IStreamQuadIterator{ifs}; qit != IStreamQuadIterator{}; ++qit) {
 *      if (qit->has_value()) {
 *          std::cout << qit->value() << std::endl;
 *      } else {
 *          std::cerr << qit->error() << std::endl;
 *      }
 * }
 * @endcode
 */
struct IStreamQuadIterator : private internal::IStreamQuadIteratorImpl {
    using flags_type = ParsingFlags;
    using state_type = ParsingState;
    using ok_type = Quad;
    using error_type = ParsingError;

    using value_type = nonstd::expected<ok_type, error_type>;
    using reference = value_type const &;
    using pointer = value_type const *;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using istream_type = std::istream;

private:
    using base = internal::IStreamQuadIteratorImpl;
    std::optional<nonstd::expected<ok_type, error_type>> cur;

public:
    /**
     * Constructs a IStreamQuadIterator from a C-like io api. That is something similar to
     * the triple (FILE *, fread, ferror) (parameters are called (stream, read, error) here).
     *
     * @param stream Pointer to a voided object, could be a FILE * cast to void * or anything else.
     * @param read see ReadFunc docs, elem_size is guaranteed to be always set to 1
     * @param error see ErrorFunc docs
     * @param flags flags to pass to the parser
     * @param initial_state optionally specifies the initial state of the parser,
     *          providing nullptr as the initial state results in the parser creating its own,fresh state
     *          instead of writing to the provided state.
     */
    IStreamQuadIterator(void *stream,
                        ReadFunc read,
                        ErrorFunc error,
                        flags_type flags = ParsingFlags::none(),
                        state_type *initial_state = nullptr)
        : base{stream, read, error, flags, initial_state},
          cur{base::next()} {
    }

    /**
     * Constructs an IStreamQuadIterator to parse an input stream in turtle syntax to quads
     *
     * @param istream turtle input stream
     * @param flags specifies the parser behaviour
     * @param initial_state optionally specifies the initial state of the parser,
     *          providing nullptr as the initial state results in the parser creating its own,fresh state
     *          instead of writing to the provided state.
     */
    explicit IStreamQuadIterator(std::istream &istream,
                                 flags_type flags = ParsingFlags::none(),
                                 state_type *initial_state = nullptr)
        : IStreamQuadIterator{&istream, &internal::istream_read, &internal::istream_error, flags, initial_state} {
    }

    IStreamQuadIterator(IStreamQuadIterator const &) = delete;
    IStreamQuadIterator(IStreamQuadIterator &&) noexcept = default;

    IStreamQuadIterator &operator=(IStreamQuadIterator const &) = delete;
    IStreamQuadIterator &operator=(IStreamQuadIterator &&) noexcept = default;

    ~IStreamQuadIterator() noexcept = default;

    reference operator*() const noexcept {
        return *cur;
    }

    pointer operator->() const noexcept {
        return &*cur;
    }

    IStreamQuadIterator &operator++() {
        cur = base::next();
        return *this;
    }

    [[nodiscard]] uint64_t current_line() const noexcept {
        return base::current_line();
    }

    [[nodiscard]] uint64_t current_column() const noexcept {
        return base::current_column();
    }

    bool operator==(std::default_sentinel_t) const noexcept {
        return !cur.has_value();
    }

    bool operator!=(std::default_sentinel_t) const noexcept {
        return cur.has_value();
    }
};

/**
 * Opens a file for fast sequential access.
 * Otherwise behaves the same as fopen.
 * Inspired by serd_fopen.
 */
FILE *fopen_fastseq(char const *path, char const *mode) noexcept;

}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_PARSER_ISTREAMQUADITERATOR_HPP
