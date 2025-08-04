#ifndef RDF4CPP_BUFWRITER_HPP
#define RDF4CPP_BUFWRITER_HPP

#include <array>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <rdf4cpp/Assert.hpp>

namespace rdf4cpp::writer {

/**
 * A type that can be used for buffered output.
 * To write bytes to a BufWriter use write_str(std::string_view, BufWriter &w) below.
 *
 * Conceptually, it is three things.
 * Some buffer-like object, a cursor into that buffer and a way to flush the buffer somewhere to make some room.
 *
 * Users of this concept write to the provided buffer in the place where the cursor points.
 * After writing the cursor should be advanced.
 * If the cursor says that there is no more room flush should be called.
 */
template<typename W>
concept BufWriter = requires (W &bw, size_t additional_cap) {
    /**
     * Type of the internal buffer
     */
    typename W::Buffer;

    /**
     * The underlying buffer, the only thing this need to be able to do is be used in flush
     */
    { bw.buffer() } -> std::same_as<typename W::Buffer &>;

    /**
     * The cursor pointing to the free space/write area inside of w.buffer()
     */
    { bw.write_area() } -> std::same_as<char *&>;

    /**
     * The size of the free space/write area inside of w.buffer()
     */
    { bw.write_area_size() } -> std::same_as<size_t &>;

    /**
     * Triggers a final flush, needs to be called after you are done using this BufWriter
     */
    { bw.finalize() } -> std::convertible_to<bool>;

    /**
     * Flushes bw.buffer() and repoints bw.cursor() to the new free space
     */
    W::flush(&bw.buffer(), &bw.write_area(), &bw.write_area_size(), additional_cap);
};

/**
 * Flushes user defined data.
 * The function's task is it to somehow make room in `buffer` and point the cursor and remaining_size to this new, free space.
 *
 * @param buffer buffer to make room in
 * @param write_area a pointer to the start of the free space, must be repointed to the start of the new free space
 * @param write_area_size the remaining size of the free space, must be set to the size of the new free space
 * @param additional_cap how much additional space is needed right now
 * @note must not throw exceptions, but is not marked noexcept so that C functions can be easily used
 */
using FlushFunc = void (*)(void *buffer, char **write_area, size_t *write_area_size, size_t additional_cap);

/**
 * The fundamental parts of a BufWriter.
 * This struct can be constructed either from a BufWriter itself
 * or manually from its raw writer.
 *
 *
 * It consists of an arbitrary, flushable/extendable buffer type (e.g. a std::string), a cursor
 * into the buffer (that describes its base address and length of the free segment) and a function to flush the buffer (i.e. to either flush to io or extend the buffer).
 * During usage callers must update the cursor to reflect the current start of the spare capacity and remaining length.
 * When the remaining length reaches zero, flush must be called using the provided buffer and cursor. Its task is to somehow make more buffer
 * room and then adjust the cursor to point to the new spare room.
 *
 * See implementation of write_str (below) for a practical usage example
 */
struct BufWriterParts {
    void *buffer; //< pointer to arbitrary buffer structure
    char **write_area; //< pointer to free space
    size_t *write_area_size; //< size of free space
    FlushFunc flush; //< function to flush the contents of buffer and update cursor to point to the new free space

    BufWriterParts() = delete;
    BufWriterParts(BufWriterParts const &) noexcept = default;
    BufWriterParts(BufWriterParts &&) noexcept = default;
    BufWriterParts &operator=(BufWriterParts const &) noexcept = default;
    BufWriterParts &operator=(BufWriterParts &&) noexcept = default;
    ~BufWriterParts() noexcept = default;

    template<BufWriter W>
    BufWriterParts(W &w) noexcept : buffer{&w.buffer()},
                                    write_area{&w.write_area()},
                                    write_area_size{&w.write_area_size()},
                                    flush{&W::flush} {
    }

    BufWriterParts(void *buffer, char **cursor, size_t *remaining_size, FlushFunc flush) noexcept : buffer{buffer},
                                                                                                    write_area{cursor},
                                                                                                    write_area_size{remaining_size},
                                                                                                    flush{flush} {
    }
};

/**
 * Serializes the given string as is
 *
 * @param str string to write
 * @param writer writer writer
 * @return true if serialization was successful, false if a call to flush was not able to make room
 *
 * @see Node::serialize for more details on the signature/usage
 *
 * @note for maintainers, this function is defined in the header because that makes serialization measurably faster
 */
inline bool write_str(std::string_view str, BufWriterParts const writer) noexcept {
    if (str.empty()) [[unlikely]] {
        // An empty std::string_view may have str.data() == nullptr
        // Calling memcpy() with a nullptr is undefined behavior **even if size == 0**
        return true;
    }

    while (true) {
        auto const max_write = std::min(str.size(), *writer.write_area_size);
        memcpy(*writer.write_area, str.data(), max_write);
        *writer.write_area += max_write;
        *writer.write_area_size -= max_write;

        if (max_write == str.size()) [[likely]] {
            break;
        }

        str.remove_prefix(max_write);

        if (writer.flush(writer.buffer, writer.write_area, writer.write_area_size, str.size()); *writer.write_area_size == 0) [[unlikely]] {
            return false;
        }
    }
    return true;
}

/**
 * (Optional) base class for serializers.
 * Encapsulates a cursor and some buffer type and makes the implementation of flush less error prone.
 *
 * @tparam CRTP derived class that is inheriting from BufWriterBase (https://en.cppreference.com/w/cpp/language/crtp)
 * @tparam Buffer buffer type to be used
 */
template<typename CRTP, typename Buffer>
struct BufWriterBase {
private:
    Buffer buffer_;
    char *write_area_ = nullptr;
    size_t write_area_size_ = 0;

public:
    template<typename ...BufferArgs>
    explicit constexpr BufWriterBase(BufferArgs &&...buffer_args) : buffer_{std::forward<BufferArgs>(buffer_args)...} {
    }

    [[nodiscard]] constexpr char *&write_area() noexcept { return write_area_; }
    [[nodiscard]] constexpr size_t &write_area_size() noexcept { return write_area_size_; }
    [[nodiscard]] constexpr Buffer &buffer() noexcept { return buffer_; }

    [[nodiscard]] constexpr char *write_area() const noexcept { return write_area_; }
    [[nodiscard]] constexpr size_t write_area_size() const noexcept { return write_area_size_; }
    [[nodiscard]] constexpr Buffer const &buffer() const noexcept { return buffer_; }

    static void flush(void *buffer, char **write_area, size_t *write_area_size, size_t additional_cap) noexcept {
        CRTP::flush_impl(*static_cast<typename CRTP::Buffer *>(buffer), *write_area, *write_area_size, additional_cap);
    }
};

struct StringBuffer {
    std::string *buffer_;

    explicit StringBuffer(std::string &buffer) noexcept : buffer_{&buffer} {
    }
};

/**
 * A serializer that serializes to a std::string
 *
 * Implements `BufWriter`
 */
struct StringWriter : BufWriterBase<StringWriter, StringBuffer> {
    using Buffer = StringBuffer;

    constexpr StringWriter(std::string &buf) noexcept : BufWriterBase<StringWriter, StringBuffer>{buf} {
        clear();
    }

    bool finalize() noexcept {
        buffer().buffer_->resize(static_cast<size_t>(write_area() - buffer().buffer_->data()));
        return true;
    }

    [[nodiscard]] std::string_view view() const noexcept {
        return std::string_view{buffer().buffer_->data(), static_cast<size_t>(write_area() - buffer().buffer_->data())};
    }

    constexpr void clear() noexcept {
        write_area() = buffer().buffer_->data();
        write_area_size() = buffer().buffer_->size();
    }

    static void flush_impl(Buffer &buffer, char *&write_area, size_t &write_area_size, size_t additional_cap) noexcept {
        auto const bytes_filled = write_area - buffer.buffer_->data();

        buffer.buffer_->resize(std::bit_ceil(buffer.buffer_->size() + additional_cap));

        write_area = buffer.buffer_->data() + bytes_filled;
        write_area_size = buffer.buffer_->size() - bytes_filled;
    }

    template<typename F>
    static std::string oneshot(F &&f) requires std::is_invocable_r_v<bool, decltype(std::forward<F>(f)), StringWriter &> {
        std::string s;
        s.resize(32);
        StringWriter w{s};

        if (!std::invoke(std::forward<F>(f), w)) {
            throw std::runtime_error{"rdf4cpp::writer::StringWriter::oneshot failed"};
        }

        w.finalize(); // cannot fail
        return s;
    }
};

struct CFileBuffer {
    FILE *file_;
    std::array<char, BUFSIZ> buffer_;

    explicit constexpr CFileBuffer(FILE *file) noexcept : file_{file} {}
};

/**
 * A serializer that serializes to a C FILE
 * It is advisable to set the internal file buffer to nullptr
 * because this serializer has an internal buffer.
 *
 * Implements `BufWriter`
 */
struct BufCFileWriter : BufWriterBase<BufCFileWriter, CFileBuffer> {
    using Buffer = CFileBuffer;

    explicit constexpr BufCFileWriter(FILE *file) noexcept : BufWriterBase<BufCFileWriter, CFileBuffer>{file} {
        write_area() = buffer().buffer_.data();
        write_area_size() = buffer().buffer_.size();
    }

    bool finalize() {
        auto const to_write = static_cast<size_t>(write_area() - buffer().buffer_.data());
        return fwrite(buffer().buffer_.data(), 1, to_write, buffer().file_) == to_write;
    }

    static void flush_impl(Buffer &buffer, char *&write_area, size_t &write_area_size, [[maybe_unused]] size_t additional_cap) noexcept {
        auto const bytes_flushed = fwrite(buffer.buffer_.data(), 1, static_cast<size_t>(write_area - buffer.buffer_.data()), buffer.file_);
        write_area -= bytes_flushed;
        write_area_size += bytes_flushed;
    }
};

struct OStreamBuffer {
    std::ostream *os_;
    std::array<char, BUFSIZ> buffer_;

    explicit constexpr OStreamBuffer(std::ostream &os) noexcept : os_{&os} {}
};

/**
 * A serializer that serializes to an std::ostream.
 * It is advisable to set the associated stream buffer to nullptr
 * because this serializer has an internal buffer.
 *
 * Implements `BufWriter`
 */
struct BufOStreamWriter : BufWriterBase<BufOStreamWriter, OStreamBuffer> {
    using Buffer = OStreamBuffer;

    explicit constexpr BufOStreamWriter(std::ostream &os) noexcept : BufWriterBase<BufOStreamWriter, OStreamBuffer>{os} {
        write_area() = buffer().buffer_.data();
        write_area_size() = buffer().buffer_.size();
    }

    bool finalize() {
        return static_cast<bool>(buffer().os_->write(buffer().buffer_.data(), static_cast<std::streamsize>(write_area() - buffer().buffer_.data())));
    }

    static void flush_impl(Buffer &buffer, char *&write_area, size_t &write_area_size, [[maybe_unused]] size_t additional_cap) noexcept {
        if (!buffer.os_->write(buffer.buffer_.data(), static_cast<std::streamsize>(write_area - buffer.buffer_.data()))) {
            return;
        }

        write_area = buffer.buffer_.data();
        write_area_size = buffer.buffer_.size();
    }
};

template<std::output_iterator<char> OutIter>
struct OutputIteratorBuffer {
    OutIter iter;
    std::array<char, BUFSIZ> buffer_{};

    explicit constexpr OutputIteratorBuffer(OutIter i) noexcept
        : iter(i) {
    }

    void write_out(char const *end) {
        char const *b = buffer_.data();
        RDF4CPP_ASSERT(b <= end && end <= buffer_.end());
        while (b != end) {
            *iter = *b;
            ++iter;
            ++b;
        }
    }
};

/**
 * A serializer that serializes to an output iterator.
 *
 * Implements `BufWriter`
 */
template<std::output_iterator<char> OutIter>
struct BufOutputIteratorWriter : BufWriterBase<BufOutputIteratorWriter<OutIter>, OutputIteratorBuffer<OutIter>> {
    using Buffer = OutputIteratorBuffer<OutIter>;

    explicit constexpr BufOutputIteratorWriter(OutIter i) noexcept
        : BufWriterBase<BufOutputIteratorWriter<OutIter>, OutputIteratorBuffer<OutIter>>{i} {

        this->write_area() = this->buffer().buffer_.data();
        this->write_area_size() = this->buffer().buffer_.size();
    }

    bool finalize() {
        this->buffer().write_out(this->write_area());
        return true;
    }

    static void flush_impl(Buffer &buffer, char *&write_area, size_t &write_area_size, [[maybe_unused]] size_t additional_cap) noexcept {
        buffer.write_out(write_area);
        write_area = buffer.buffer_.data();
        write_area_size = buffer.buffer_.size();
    }
};
} // namespace rdf4cpp::writer

#endif  // RDF4CPP_BUFWRITER_HPP
