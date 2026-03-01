#include "FormatGuess.hpp"

#include <algorithm>
#include <string>

#include <uni_algo/all.h>

namespace rdf4cpp::parser {

    // --- helpers ---

    struct SplitLine {
        std::string_view line;
        std::string_view rest;
    };

    // RDF syntax delimiters are ASCII bytes (0x00-0x7F) which never appear as
    // UTF-8 continuation bytes.  Byte-level scanning for these markers is safe
    // in valid UTF-8.

    static bool is_ascii_ws(char const c) noexcept {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static std::string_view ltrim_ascii_whitespace(std::string_view const sv) noexcept {
        auto const it = std::ranges::find_if_not(sv, is_ascii_ws);
        return sv.substr(static_cast<size_t>(it - sv.begin()));
    }

    static std::string_view trim_ascii_whitespace(std::string_view const sv) noexcept {
        auto result = ltrim_ascii_whitespace(sv);
        while (!result.empty() && is_ascii_ws(result.back())) {
            result.remove_suffix(1);
        }
        return result;
    }

    /// Split into first line and everything after the newline. If no newline, rest is empty.
    static SplitLine split_next_line(std::string_view const sv) noexcept {
        auto const eol = sv.find('\n');
        if (eol == std::string_view::npos) {
            return {.line = sv, .rest = {}};
        }
        return {.line = sv.substr(0, eol), .rest = sv.substr(eol + 1)};
    }

    static std::string_view skip_whitespace_and_bom(std::string_view const sv) noexcept {
        // skip UTF-8 BOM
        if (sv.starts_with("\xEF\xBB\xBF")) {
            return ltrim_ascii_whitespace(sv.substr(3));
        }
        return ltrim_ascii_whitespace(sv);
    }

    static bool starts_with_icase(std::string_view const haystack, std::string_view const needle) {
        if (haystack.size() < needle.size()) {
            return false;
        }
        return una::cases::to_lowercase_utf8(haystack.substr(0, needle.size())) == una::cases::to_lowercase_utf8(needle);
    }

    static bool contains(std::string_view const haystack, std::string_view const needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

    static bool contains_icase(std::string_view const haystack, std::string_view const needle) {
        return static_cast<bool>(una::caseless::find_utf8(haystack, needle));
    }

    // --- extension mapping ---

    FormatGuess guess_format_from_extension(std::string_view const extension) noexcept {
        auto const ext = una::cases::to_lowercase_utf8(extension);

        if (ext == ".ttl" || ext == ".turtle") {
            return {ParsingFlag::Turtle, GuessConfidence::High};
        }
        if (ext == ".nt" || ext == ".ntriples") {
            return {ParsingFlag::NTriples, GuessConfidence::High};
        }
        if (ext == ".nq" || ext == ".nquads") {
            return {ParsingFlag::NQuads, GuessConfidence::High};
        }
        if (ext == ".trig") {
            return {ParsingFlag::TriG, GuessConfidence::High};
        }
        if (ext == ".rdf") {
            return {ParsingFlag::RdfXml, GuessConfidence::High};
        }
        if (ext == ".owx") {
            return {ParsingFlag::OwlXml, GuessConfidence::High};
        }
        if (ext == ".jsonld") {
            return {ParsingFlag::JsonLd, GuessConfidence::High};
        }

        // ambiguous extensions — need content sniffing
        if (ext == ".owl" || ext == ".xml") {
            return {ParsingFlag::RdfXml, GuessConfidence::Low};
        }

        return {ParsingFlag::Auto, GuessConfidence::None};
    }

    FormatGuess guess_format_from_path(std::string_view const file_path) noexcept {
        // find last path separator
        auto const last_sep = file_path.find_last_of("/\\");
        auto const filename = (last_sep != std::string_view::npos) ? file_path.substr(last_sep + 1) : file_path;

        // find the last dot in the filename
        auto const dot_pos = filename.rfind('.');
        if (dot_pos == std::string_view::npos) {
            return {ParsingFlag::Auto, GuessConfidence::None};
        }

        return guess_format_from_extension(filename.substr(dot_pos));
    }

    // --- content sniffing ---

    static bool has_trig_markers(std::string_view const content) {
        // Look for GRAPH keyword or { } blocks outside of string literals
        if (contains_icase(content, "GRAPH")) {
            return true;
        }

        // Look for a pattern like IRI/prefixed-name followed by {
        // or just { at the start of a line (default graph block)
        bool in_string = false;
        char string_delim = 0;
        auto cursor = content;
        while (!cursor.empty()) {
            char const c = cursor.front();
            cursor.remove_prefix(1);
            if (in_string) {
                if (c == '\\' && !cursor.empty()) {
                    cursor.remove_prefix(1);  // skip escaped char
                    continue;
                }
                if (c == string_delim) {
                    in_string = false;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_string = true;
                string_delim = c;
                continue;
            }
            if (c == '{') {
                return true;
            }
        }
        return false;
    }

    static FormatGuess sniff_xml_content(std::string_view const content) {
        // Check for OWL/XML first — more specific markers.
        // OWL/XML uses <Ontology> root element (not <rdf:RDF>) and may still
        // declare xmlns:rdf as a namespace prefix, so checking OWL/XML before
        // RDF/XML avoids false positives.
        bool const has_ontology_root = contains(content, "<Ontology");
        bool const has_owl_ns = contains(content, "xmlns=\"http://www.w3.org/2002/07/owl#\"");
        bool const has_rdf_root = contains(content, "<rdf:RDF");
        bool const has_rdf_desc = contains(content, "<rdf:Description");

        if (has_ontology_root && !has_rdf_root) {
            return {ParsingFlag::OwlXml, GuessConfidence::High};
        }
        if (has_owl_ns && !has_rdf_root && !has_rdf_desc) {
            return {ParsingFlag::OwlXml, GuessConfidence::High};
        }
        if (has_rdf_root || has_rdf_desc) {
            return {ParsingFlag::RdfXml, GuessConfidence::High};
        }
        if (contains(content, "xmlns:rdf=")) {
            return {ParsingFlag::RdfXml, GuessConfidence::Medium};
        }
        // Generic XML — could be RDF/XML, return low-confidence guess
        return {ParsingFlag::RdfXml, GuessConfidence::Low};
    }

    static FormatGuess sniff_json_content(std::string_view const content) {
        if (contains(content, "\"@context\"") || contains(content, "\"@id\"") || contains(content, "\"@graph\"")) {
            return {ParsingFlag::JsonLd, GuessConfidence::High};
        }
        return {ParsingFlag::Auto, GuessConfidence::None};
    }

    static FormatGuess sniff_ntriples_or_nquads(std::string_view const content) {
        // Scan lines looking for N-Triples/N-Quads patterns:
        // Lines of <iri> <iri> <obj> . (3 terms = NT, 4 terms = NQ)
        bool found_4_terms = false;
        bool found_any_triple = false;

        auto remaining = content;
        while (!remaining.empty()) {
            auto const [line_raw, rest] = split_next_line(remaining);
            remaining = rest;
            auto const line = trim_ascii_whitespace(line_raw);

            // skip empty lines and comments
            if (line.empty() || line.front() == '#') {
                continue;
            }

            // Count terms by walking through the line, handling # comments
            // that appear outside IRIs and literals correctly.
            int term_count = 0;
            bool found_dot = false;
            auto cursor = line;
            while (!cursor.empty()) {
                // skip whitespace
                cursor = ltrim_ascii_whitespace(cursor);
                if (cursor.empty()) {
                    break;
                }

                if (char const c = cursor.front(); c == '.') {
                    found_dot = true;
                    cursor.remove_prefix(1);
                    break;
                } else if (c == '#') {
                    // comment at the top level (outside IRI/literal) — not valid N-Triples
                    return {ParsingFlag::Auto, GuessConfidence::None};
                } else if (c == '<') {
                    // IRI — find closing >
                    auto const close = cursor.find('>');
                    if (close == std::string_view::npos) {
                        break;
                    }
                    cursor.remove_prefix(close + 1);
                    ++term_count;
                } else if (cursor.starts_with("_:")) {
                    // blank node — skip to the next whitespace
                    auto const ws = std::ranges::find_if(cursor, is_ascii_ws);
                    cursor.remove_prefix(static_cast<size_t>(ws - cursor.begin()));
                    ++term_count;
                } else if (c == '"') {
                    // literal — find unescaped closing quote, then skip datatype/lang
                    cursor.remove_prefix(1);
                    while (!cursor.empty()) {
                        if (cursor.front() == '\\') {
                            cursor.remove_prefix(std::min<size_t>(2, cursor.size()));
                            continue;
                        }
                        if (cursor.front() == '"') {
                            break;
                        }
                        cursor.remove_prefix(1);
                    }
                    if (!cursor.empty()) {
                        cursor.remove_prefix(1);  // skip closing quote
                    }
                    // skip ^^<datatype> or @lang (which may contain # inside <...>)
                    if (cursor.starts_with("^^")) {
                        cursor.remove_prefix(2);
                        if (!cursor.empty() && cursor.front() == '<') {
                            if (auto const close = cursor.find('>'); close != std::string_view::npos) {
                                cursor.remove_prefix(close + 1);
                            }
                        } else {
                            auto const end = std::ranges::find_if(cursor, [](char const ch) noexcept {
                                return is_ascii_ws(ch) || ch == '.';
                            });
                            cursor.remove_prefix(static_cast<size_t>(end - cursor.begin()));
                        }
                    } else if (!cursor.empty() && cursor.front() == '@') {
                        auto const end = std::ranges::find_if(cursor, [](char const ch) noexcept {
                            return is_ascii_ws(ch) || ch == '.';
                        });
                        cursor.remove_prefix(static_cast<size_t>(end - cursor.begin()));
                    }
                    ++term_count;
                } else {
                    // unexpected character for N-Triples/N-Quads
                    return {ParsingFlag::Auto, GuessConfidence::None};
                }
            }

            if (!found_dot) {
                return {ParsingFlag::Auto, GuessConfidence::None};
            }
            if (term_count == 4) {
                found_4_terms = true;
            }
            if (term_count >= 3) {
                found_any_triple = true;
            }
            if (term_count < 3 || term_count > 4) {
                return {ParsingFlag::Auto, GuessConfidence::None};
            }
        }

        if (!found_any_triple) {
            return {ParsingFlag::Auto, GuessConfidence::None};
        }
        if (found_4_terms) {
            return {ParsingFlag::NQuads, GuessConfidence::Medium};
        }
        return {ParsingFlag::NTriples, GuessConfidence::Medium};
    }

    static std::string_view skip_comments(std::string_view const sv) {
        // skip lines starting with # (comments in N-Triples/Turtle/TriG)
        auto cursor = sv;
        while (!cursor.empty() && cursor.front() == '#') {
            auto const eol = cursor.find('\n');
            if (eol == std::string_view::npos) {
                return {};
            }
            cursor = ltrim_ascii_whitespace(cursor.substr(eol + 1));
        }
        return cursor;
    }

    static FormatGuess sniff_bracket_content(std::string_view const content, std::string_view const full_content) {
        auto const after_bracket = ltrim_ascii_whitespace(content.substr(1));
        // JSON arrays start with `[` followed by `{`, `"`, `[`, number, true, false, null
        // Turtle/TriG blank nodes: `[]` or `[ predicate object ]`
        if (!after_bracket.empty() && (after_bracket.front() == ']' || after_bracket.front() == '<' || after_bracket.front() == '_')) {
            // Likely Turtle/TriG blank node
            if (has_trig_markers(full_content)) {
                return {ParsingFlag::TriG, GuessConfidence::Medium};
            }
            return {ParsingFlag::Turtle, GuessConfidence::Low};
        }
        return sniff_json_content(content);
    }

    static FormatGuess sniff_brace_content(std::string_view const content) {
        auto const after_brace = ltrim_ascii_whitespace(content.substr(1));
        if (after_brace.empty() || after_brace.front() == '"') {
            return sniff_json_content(content);
        }
        // Likely TriG — `{` followed by non-JSON content
        return {ParsingFlag::TriG, GuessConfidence::Medium};
    }

    static FormatGuess sniff_turtle_or_trig_markers(std::string_view const content, std::string_view const full_content) {
        static constexpr std::string_view turtle_markers = ";,()[]{}";
        bool has_turtle_marker = false;
        bool in_iri = false;
        bool in_string = false;
        char string_delim = 0;
        char prev_char = 0;

        auto cursor = content;
        while (!cursor.empty()) {
            char const c = cursor.front();
            cursor.remove_prefix(1);
            if (in_string) {
                if (c == '\\' && !cursor.empty()) {
                    prev_char = cursor.front();
                    cursor.remove_prefix(1);
                    continue;
                }
                if (c == string_delim) {
                    in_string = false;
                }
                prev_char = c;
                continue;
            }
            if (in_iri) {
                if (c == '>') {
                    in_iri = false;
                }
                prev_char = c;
                continue;
            }
            if (c == '<') {
                in_iri = true;
                prev_char = c;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_string = true;
                string_delim = c;
                prev_char = c;
                continue;
            }
            // Turtle/TriG syntax markers not valid in N-Triples
            if (turtle_markers.find(c) != std::string_view::npos) {
                has_turtle_marker = true;
                break;
            }
            // bare `a` surrounded by whitespace = Turtle shorthand for rdf:type
            if (c == 'a' && (prev_char == ' ' || prev_char == '\t') && !cursor.empty() && (cursor.front() == ' ' || cursor.front() == '\t'))
            {
                has_turtle_marker = true;
                break;
            }
            prev_char = c;
        }

        if (has_turtle_marker) {
            if (has_trig_markers(full_content)) {
                return {ParsingFlag::TriG, GuessConfidence::Low};
            }
            return {ParsingFlag::Turtle, GuessConfidence::Low};
        }

        return {ParsingFlag::Auto, GuessConfidence::None};
    }

    FormatGuess guess_format_from_content(std::string_view const prefix) noexcept {
        auto const full_content = skip_whitespace_and_bom(prefix);
        if (full_content.empty()) {
            return {ParsingFlag::Auto, GuessConfidence::None};
        }

        // Skip leading comment lines for the first-byte checks
        auto const content = skip_comments(full_content);
        if (content.empty()) {
            return {ParsingFlag::Auto, GuessConfidence::None};
        }

        // Phase 1: deterministic checks

        // XML-based formats
        if (content.starts_with("<?xml") || content.starts_with("<rdf:RDF") || content.starts_with("<rdf:")) {
            return sniff_xml_content(content);
        }

        // JSON-based formats — but `{` can also be a TriG default graph block,
        // and `[` can be a TriG blank node graph name or a Turtle blank node property list.
        if (content.front() == '[') {
            return sniff_bracket_content(content, full_content);
        }
        if (content.front() == '{') {
            return sniff_brace_content(content);
        }

        // Turtle directives (case-sensitive @prefix/@base)
        if (content.starts_with("@prefix") || content.starts_with("@base")) {
            if (has_trig_markers(full_content)) {
                return {ParsingFlag::TriG, GuessConfidence::Medium};
            }
            return {ParsingFlag::Turtle, GuessConfidence::High};
        }

        // SPARQL-style PREFIX/BASE (case-insensitive)
        if (starts_with_icase(content, "PREFIX") || starts_with_icase(content, "BASE")) {
            if (has_trig_markers(full_content)) {
                return {ParsingFlag::TriG, GuessConfidence::Medium};
            }
            return {ParsingFlag::Turtle, GuessConfidence::Medium};
        }

        // Phase 2: try N-Triples / N-Quads line-based detection
        if (content.front() == '<' || content.starts_with("_:")) {
            if (auto const result = sniff_ntriples_or_nquads(full_content); result.is_known()) {
                return result;
            }
            // If N-Triples detection failed, the content starts with `<` or `_:`
            // which are valid Turtle starts too — fall through to Phase 3
        }

        // Phase 3: check for Turtle/TriG syntax markers in content that didn't
        // match any earlier patterns (e.g., Turtle without @prefix directives)
        return sniff_turtle_or_trig_markers(content, full_content);
    }

    FormatGuess guess_format(std::string_view const file_path, std::string_view const prefix) noexcept {
        auto const ext_guess = guess_format_from_path(file_path);
        auto const content_guess = guess_format_from_content(prefix);

        // If extension gives a strong match and no content sniffing needed
        if (ext_guess.confidence == GuessConfidence::High) {
            // Check if content agrees for Certain confidence
            if (content_guess.is_known() && content_guess.syntax == ext_guess.syntax) {
                return {ext_guess.syntax, GuessConfidence::Certain};
            }
            // Extension is high confidence — trust it even if the content is ambiguous
            return ext_guess;
        }

        // Low confidence extension (e.g., .owl, .xml) — need content disambiguation
        if (ext_guess.confidence == GuessConfidence::Low) {
            if (content_guess.is_known()) {
                // Content overrides ambiguous extension
                return content_guess;
            }
            // Content inconclusive, use extension guess
            return ext_guess;
        }

        // No extension match — rely on content
        if (content_guess.is_known()) {
            return content_guess;
        }

        return {ParsingFlag::Auto, GuessConfidence::None};
    }

}  // namespace rdf4cpp::parser
