#include "RDFFileParser.hpp"

#include <rdf4cpp/parser/FormatGuess.hpp>
#include <rdf4cpp/parser/IStreamQuadIteratorSerdImpl.hpp>

#include <stdexcept>
#include <vector>

namespace rdf4cpp::parser {

static constexpr size_t peek_size = 4096;

RDFFileParser::RDFFileParser(const std::string &file_path, flags_type flags, state_type *state)
    : file_path_(file_path), flags_(flags), state_(state) {
}
RDFFileParser::RDFFileParser(std::string &&file_path, flags_type flags, state_type *state)
    : file_path_(std::move(file_path)), flags_(flags), state_(state) {
}

RDFFileParser::iterator RDFFileParser::begin() const {
    FILE *stream = fopen_fastseq(file_path_.c_str(), "r");
    if (stream == nullptr) {
        throw std::system_error{errno, std::system_category()};
    }

    auto flags = flags_;

    if (flags.get_syntax() == ParsingFlag::Auto) {
        // Peek content for sniffing
        std::vector<char> buf(peek_size);
        size_t bytes_read = fread(buf.data(), 1, peek_size, stream);
        buf.resize(bytes_read);

        // Rewind the stream so IStreamQuadIterator reads from start
        if (fseek(stream, 0, SEEK_SET) != 0) {
            fclose(stream);
            throw std::runtime_error("Failed to rewind file stream for format detection");
        }

        std::string_view const prefix{buf.data(), buf.size()};
        auto guess = guess_format(file_path_, prefix);

        auto resolved = guess.is_known() ? guess.syntax : ParsingFlag::Turtle;

        if (resolved == ParsingFlag::OwlXml) {
            fclose(stream);
            throw std::runtime_error("OWL/XML format is not supported. Please convert to RDF/XML or Turtle.");
        }
        if (resolved == ParsingFlag::JsonLd) {
            fclose(stream);
            throw std::runtime_error("JSON-LD format is not supported.");
        }

        flags = flags.with_syntax(resolved);
    }

    return {std::move(stream), flags, state_};
}

std::default_sentinel_t RDFFileParser::end() const noexcept {
    return {};
}

RDFFileParser::iterator::iterator()
    : stream_(nullptr), iter_(nullptr) {
}
RDFFileParser::iterator::iterator(FILE *&&stream,
                                  flags_type flags,
                                  state_type *state)
    : stream_(stream),
      iter_(std::make_unique<IStreamQuadIterator>(stream_, reinterpret_cast<ReadFunc>(&fread), reinterpret_cast<ErrorFunc>(&ferror),
                                            reinterpret_cast<EOFFunc>(feof), flags, state)) {
}
RDFFileParser::iterator::~iterator() noexcept {
    fclose(stream_);
}
RDFFileParser::iterator::reference RDFFileParser::iterator::operator*() const noexcept {
    return (*iter_).operator*();
}
RDFFileParser::iterator::pointer RDFFileParser::iterator::operator->() const noexcept {
    return (*iter_).operator->();
}
RDFFileParser::iterator &RDFFileParser::iterator::operator++() {
    ++(*iter_);
    return *this;
}
bool RDFFileParser::iterator::operator==(const RDFFileParser::iterator &other) const noexcept {
    return iter_ == other.iter_;
}
FormatGuess RDFFileParser::iterator::detected_format() const noexcept {
    if (iter_) {
        return iter_->detected_format();
    }
    return {};
}
bool operator==(const RDFFileParser::iterator &iter, std::default_sentinel_t s) noexcept {
    return (*iter.iter_) == s;
}
bool operator==(std::default_sentinel_t s, const RDFFileParser::iterator &iter) noexcept {
    return iter == s;
}
}  // namespace rdf4cpp::parser
