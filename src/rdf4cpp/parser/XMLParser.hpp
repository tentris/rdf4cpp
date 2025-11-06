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

    private:
        struct Impl;

        std::unique_ptr<Impl> impl_;
        std::optional<nonstd::expected<ok_type, error_type>> cur_;

    public:
        explicit XMLQuadIterator(std::istream& stream);

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
