#include <rdf4cpp/parser/IStreamQuadIterator.hpp>

#include <cstdio>

#if __has_include(<fcntl.h>)
#include <fcntl.h>
#endif //__has_include

namespace rdf4cpp::parser {

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
