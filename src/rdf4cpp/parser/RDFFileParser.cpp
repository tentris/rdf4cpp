#include "RDFFileParser.hpp"


#include <rdf4cpp/parser/IStreamQuadIteratorSerdImpl.hpp>

namespace rdf4cpp::parser {
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

    return {std::move(stream), flags_, state_};
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
bool operator==(const RDFFileParser::iterator &iter, std::default_sentinel_t s) noexcept {
    return (*iter.iter_) == s;
}
bool operator==(std::default_sentinel_t s, const RDFFileParser::iterator &iter) noexcept {
    return iter == s;
}
}  // namespace rdf4cpp::parser
