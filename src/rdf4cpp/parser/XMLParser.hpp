#ifndef RDF4CPP_XMLPARSER_HPP
#define RDF4CPP_XMLPARSER_HPP

#include <iterator>
#include <memory>

#include <rdf4cpp/Expected.hpp>

#include <rdf4cpp/Quad.hpp>

#include <rdf4cpp/parser/ParsingError.hpp>
#include <rdf4cpp/parser/ParsingFlags.hpp>
#include <rdf4cpp/parser/ParsingState.hpp>
#include <rdf4cpp/IRIFactory.hpp>

namespace rdf4cpp::parser {
    struct XMLQuadIterator {
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

        /**
         * Identical semantics to fread.
         * Uses stream to read at most count elements of size element_size into buffer.
         *
         * @param buffer pointer to buffer with at least count elements of size elem_size
         * @param elem_size sizeof each element
         * @param count number of elements to read
         * @param stream pointer to any object.
         * @return number of elements read
         */
        using ReadFunc = size_t (*)(void *buffer, size_t elem_size, size_t count, void *stream);

        /**
         * Identical semantics to ferror.
         *
         * @param stream pointer to any object
         * @return nonzero value if there is an error in stream, zero value otherwise
         */
        using ErrorFunc = int (*)(void *stream);

        /**
         * Identical semantics to feof.
         *
         *
         * @param stream pointer to any object
         * @return nonzero value if there is an error in stream, zero value otherwise
         */
        using EOFFunc = int (*)(void *stream);

    private:
        struct Impl;

        std::unique_ptr<Impl> impl_;
        std::optional<nonstd::expected<ok_type, error_type>> cur_;

    public:
        XMLQuadIterator(void *stream, ReadFunc read, ErrorFunc error, EOFFunc eof, state_type* state = nullptr);
        explicit XMLQuadIterator(std::istream& stream, state_type* state = nullptr);

        XMLQuadIterator(XMLQuadIterator&&) noexcept = delete;
        XMLQuadIterator& operator=(XMLQuadIterator&&) noexcept = delete;

        XMLQuadIterator(XMLQuadIterator const &) = delete;
        XMLQuadIterator& operator=(XMLQuadIterator const &) = delete;

        ~XMLQuadIterator() noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;
        XMLQuadIterator &operator++();

        bool operator==(std::default_sentinel_t) const noexcept;
    };
}

#endif  //RDF4CPP_XMLPARSER_H
