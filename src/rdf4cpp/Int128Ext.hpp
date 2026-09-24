#ifndef RDF4CPP_INT128EXT_HPP
#define RDF4CPP_INT128EXT_HPP

// This code lives here to avoid a circular include between <rdf4cpp/Int128.hpp> and <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>.
// We do not provide operator<< or std::formatter to avoid issues if or when the stdlib adds those itself in the future.

#include <rdf4cpp/Int128.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

namespace rdf4cpp {

[[nodiscard]] inline std::string to_string(Int128 const value) {
    return writer::StringWriter::oneshot([value](writer::StringWriter &w) {
        return datatypes::registry::util::to_chars_canonical(value, w);
    });
}

[[nodiscard]] inline std::string to_string(UInt128 const value) {
    return writer::StringWriter::oneshot([value](writer::StringWriter &w) {
        return datatypes::registry::util::to_chars_canonical(value, w);
    });
}

} // namespace rdf4cpp

#endif // RDF4CPP_INT128EXT_HPP
