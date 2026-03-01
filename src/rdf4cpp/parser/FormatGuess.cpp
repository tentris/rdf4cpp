#include "FormatGuess.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace rdf4cpp::parser {

    // --- helpers ---

    static std::string to_lower(std::string_view sv) {
        std::string s{sv};
        std::ranges::transform(s, s.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return s;
    }

    static std::string_view skip_whitespace_and_bom(std::string_view sv) {
        // skip UTF-8 BOM
        if (sv.size() >= 3 && sv[0] == '\xEF' && sv[1] == '\xBB' && sv[2] == '\xBF') {
            sv.remove_prefix(3);
        }
        // skip leading whitespace
        while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\n' || sv.front() == '\r')) {
            sv.remove_prefix(1);
        }
        return sv;
    }

    static bool starts_with_icase(std::string_view haystack, std::string_view needle) {
        if (haystack.size() < needle.size()) {
            return false;
        }
        for (size_t i = 0; i < needle.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(haystack[i])) != std::tolower(static_cast<unsigned char>(needle[i]))) {
                return false;
            }
        }
        return true;
    }

    static bool contains(std::string_view haystack, std::string_view needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

    static bool contains_icase(std::string_view haystack, std::string_view needle) {
        if (needle.size() > haystack.size()) {
            return false;
        }
        auto lower_hay = to_lower(haystack);
        auto lower_needle = to_lower(needle);
        return lower_hay.find(lower_needle) != std::string::npos;
    }

    // --- extension mapping ---

    FormatGuess guess_format_from_extension(std::string_view extension) noexcept {
        auto ext = to_lower(extension);

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

    FormatGuess guess_format_from_path(std::string_view file_path) noexcept {
        // find last path separator
        auto const last_sep = file_path.find_last_of("/\\");
        auto const filename = (last_sep != std::string_view::npos) ? file_path.substr(last_sep + 1) : file_path;

        // find last dot in filename
        auto const dot_pos = filename.rfind('.');
        if (dot_pos == std::string_view::npos) {
            return {ParsingFlag::Auto, GuessConfidence::None};
        }

        return guess_format_from_extension(filename.substr(dot_pos));
    }

    // --- content sniffing ---

    static bool has_trig_markers(std::string_view content) {
        // Look for GRAPH keyword or { } blocks outside of string literals
        // Simple heuristic: look for GRAPH keyword or standalone { not inside quotes
        if (contains_icase(content, "GRAPH")) {
            return true;
        }

        // Look for pattern like IRI/prefixed-name followed by {
        // or just { at start of a line (default graph block)
        bool in_string = false;
        char string_delim = 0;
        for (size_t i = 0; i < content.size(); ++i) {
            char c = content[i];
            if (in_string) {
                if (c == '\\' && i + 1 < content.size()) {
                    ++i;  // skip escaped char
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

    static FormatGuess sniff_xml_content(std::string_view content) {
        // Check for OWL/XML first — more specific markers.
        // OWL/XML uses <Ontology> root element (not <rdf:RDF>) and may still
        // declare xmlns:rdf as a namespace prefix, so checking OWL/XML before
        // RDF/XML avoids false positives.
        bool has_ontology_root = contains(content, "<Ontology");
        bool has_owl_ns = contains(content, "xmlns=\"http://www.w3.org/2002/07/owl#\"");
        bool has_rdf_root = contains(content, "<rdf:RDF");
        bool has_rdf_desc = contains(content, "<rdf:Description");

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

    static FormatGuess sniff_json_content(std::string_view content) {
        if (contains(content, "\"@context\"") || contains(content, "\"@id\"") || contains(content, "\"@graph\"")) {
            return {ParsingFlag::JsonLd, GuessConfidence::High};
        }
        return {ParsingFlag::Auto, GuessConfidence::None};
    }

    static FormatGuess sniff_ntriples_or_nquads(std::string_view content) {
        // Scan lines looking for N-Triples/N-Quads patterns:
        // Lines of <iri> <iri> <obj> . (3 terms = NT, 4 terms = NQ)
        bool found_4_terms = false;
        bool found_any_triple = false;

        size_t pos = 0;
        while (pos < content.size()) {
            // find end of line
            auto eol = content.find('\n', pos);
            auto line = content.substr(pos, eol == std::string_view::npos ? std::string_view::npos : eol - pos);
            pos = (eol == std::string_view::npos) ? content.size() : eol + 1;

            // trim
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
                line.remove_prefix(1);
            }
            while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
                line.remove_suffix(1);
            }

            // skip empty lines and comments
            if (line.empty() || line.front() == '#') {
                continue;
            }

            // Count terms by walking through the line, handling # comments
            // that appear outside of IRIs and literals correctly.
            int term_count = 0;
            bool found_dot = false;
            size_t i = 0;
            while (i < line.size()) {
                // skip whitespace
                while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
                    ++i;
                }
                if (i >= line.size()) {
                    break;
                }
                char c = line[i];

                if (c == '.') {
                    found_dot = true;
                    ++i;
                    // skip trailing whitespace and optional comment after .
                    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
                        ++i;
                    }
                    if (i < line.size() && line[i] == '#') {
                        // inline comment after dot — valid
                    }
                    break;
                } else if (c == '#') {
                    // comment at top level (outside IRI/literal) — not valid N-Triples
                    // unless we already found the dot
                    return {ParsingFlag::Auto, GuessConfidence::None};
                } else if (c == '<') {
                    // IRI — find closing >
                    auto close = line.find('>', i);
                    if (close == std::string_view::npos) {
                        break;
                    }
                    i = close + 1;
                    ++term_count;
                } else if (c == '_' && i + 1 < line.size() && line[i + 1] == ':') {
                    // blank node — skip to next whitespace
                    while (i < line.size() && line[i] != ' ' && line[i] != '\t') {
                        ++i;
                    }
                    ++term_count;
                } else if (c == '"') {
                    // literal — find unescaped closing quote, then skip datatype/lang
                    ++i;
                    while (i < line.size()) {
                        if (line[i] == '\\') {
                            i += 2;
                            continue;
                        }
                        if (line[i] == '"') {
                            break;
                        }
                        ++i;
                    }
                    if (i < line.size()) {
                        ++i;  // skip closing quote
                    }
                    // skip ^^<datatype> or @lang (which may contain # inside <...>)
                    if (i + 1 < line.size() && line[i] == '^' && line[i + 1] == '^') {
                        i += 2;
                        if (i < line.size() && line[i] == '<') {
                            auto close = line.find('>', i);
                            if (close != std::string_view::npos) {
                                i = close + 1;
                            }
                        } else {
                            while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '.') {
                                ++i;
                            }
                        }
                    } else if (i < line.size() && line[i] == '@') {
                        while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '.') {
                            ++i;
                        }
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

    static std::string_view skip_comments(std::string_view sv) {
        // skip lines starting with # (comments in N-Triples/Turtle/TriG)
        while (!sv.empty() && sv.front() == '#') {
            auto eol = sv.find('\n');
            if (eol == std::string_view::npos) {
                return {};
            }
            sv.remove_prefix(eol + 1);
            // skip whitespace after comment line
            while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\n' || sv.front() == '\r')) {
                sv.remove_prefix(1);
            }
        }
        return sv;
    }

    FormatGuess guess_format_from_content(std::string_view prefix) noexcept {
        auto full_content = skip_whitespace_and_bom(prefix);
        if (full_content.empty()) {
            return {ParsingFlag::Auto, GuessConfidence::None};
        }

        // Skip leading comment lines for the first-byte checks
        auto content = skip_comments(full_content);
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
            auto after_bracket = content.substr(1);
            while (!after_bracket.empty()
                   && (after_bracket.front() == ' ' || after_bracket.front() == '\t' || after_bracket.front() == '\n'
                       || after_bracket.front() == '\r'))
            {
                after_bracket.remove_prefix(1);
            }
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
        if (content.front() == '{') {
            auto after_brace = content.substr(1);
            while (!after_brace.empty()
                   && (after_brace.front() == ' ' || after_brace.front() == '\t' || after_brace.front() == '\n'
                       || after_brace.front() == '\r'))
            {
                after_brace.remove_prefix(1);
            }
            if (after_brace.empty() || after_brace.front() == '"') {
                return sniff_json_content(content);
            }
            // Likely TriG — `{` followed by non-JSON content
            return {ParsingFlag::TriG, GuessConfidence::Medium};
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
        if (content.front() == '<' || (content.front() == '_' && content.size() > 1 && content[1] == ':')) {
            auto result = sniff_ntriples_or_nquads(full_content);
            if (result.is_known()) {
                return result;
            }
            // If N-Triples detection failed, the content starts with `<` or `_:`
            // which are valid Turtle starts too — fall through to Phase 3
        }

        // Phase 3: check for Turtle/TriG syntax markers in content that didn't
        // match any earlier patterns (e.g. Turtle without @prefix directives)
        {
            bool has_turtle_marker = false;
            bool in_iri = false;
            bool in_string = false;
            char string_delim = 0;

            for (size_t i = 0; i < content.size(); ++i) {
                char c = content[i];
                if (in_string) {
                    if (c == '\\' && i + 1 < content.size()) {
                        ++i;
                        continue;
                    }
                    if (c == string_delim) {
                        in_string = false;
                    }
                    continue;
                }
                if (in_iri) {
                    if (c == '>') {
                        in_iri = false;
                    }
                    continue;
                }
                if (c == '<') {
                    in_iri = true;
                    continue;
                }
                if (c == '"' || c == '\'') {
                    in_string = true;
                    string_delim = c;
                    continue;
                }
                // Turtle/TriG syntax markers not valid in N-Triples
                if (c == ';' || c == ',' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}') {
                    has_turtle_marker = true;
                    break;
                }
                // bare `a` surrounded by whitespace = Turtle shorthand for rdf:type
                if (c == 'a' && i > 0 && (content[i - 1] == ' ' || content[i - 1] == '\t') && i + 1 < content.size()
                    && (content[i + 1] == ' ' || content[i + 1] == '\t'))
                {
                    has_turtle_marker = true;
                    break;
                }
            }

            if (has_turtle_marker) {
                if (has_trig_markers(full_content)) {
                    return {ParsingFlag::TriG, GuessConfidence::Low};
                }
                return {ParsingFlag::Turtle, GuessConfidence::Low};
            }
        }

        return {ParsingFlag::Auto, GuessConfidence::None};
    }

    FormatGuess guess_format(std::string_view file_path, std::string_view prefix) noexcept {
        auto ext_guess = guess_format_from_path(file_path);
        auto content_guess = guess_format_from_content(prefix);

        // If extension gives a strong match and no content sniffing needed
        if (ext_guess.confidence == GuessConfidence::High) {
            // Check if content agrees for Certain confidence
            if (content_guess.is_known() && content_guess.syntax == ext_guess.syntax) {
                return {ext_guess.syntax, GuessConfidence::Certain};
            }
            // Extension is high confidence — trust it even if content is ambiguous
            return ext_guess;
        }

        // Low confidence extension (e.g. .owl, .xml) — need content disambiguation
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
