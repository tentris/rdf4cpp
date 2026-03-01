# FormatGuess — RDF Serialization Format Detection

## Purpose

`FormatGuess` provides automatic detection of RDF serialization formats from
file extensions and/or a content prefix (the first few hundred to few thousand
bytes). It returns a `FormatGuess` consisting of a `ParsingFlag` (the detected
syntax) and a `GuessConfidence` level.

## Detection Strategy

Three entry points, in order of specificity:

| Function                        | Input          | Returned confidence levels                                                                                                                                         |
|---------------------------------|----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `guess_format_from_extension()` | file extension | High (`.ttl`, `.nt`, …), Low (`.owl`, `.xml`), or None                                                                                                             |
| `guess_format_from_content()`   | byte prefix    | High (XML root, `@prefix`, JSON-LD keywords), Medium (`PREFIX`/`BASE`, N-Triples/N-Quads grammar, TriG markers), Low (Turtle syntax markers, generic XML), or None |
| `guess_format()`                | path + prefix  | Certain (extension + content agree), otherwise delegates to the above                                                                                              |

`guess_format()` combines extension and content results: when both agree the
confidence is boosted to **Certain**.

## Content Sniffing Phases

`guess_format_from_content()` inspects the prefix in three ordered phases.
Processing stops at the first match.

### Phase 1 — Deterministic Checks

Fast tests on the first non-whitespace, non-comment bytes:

* `<?xml`, `<rdf:RDF`, `<rdf:` → XML sub-classifier (`sniff_xml_content`)
* `[` / `{` → disambiguate JSON-LD vs Turtle/TriG based on what follows
* `@prefix` / `@base` → Turtle (or TriG if GRAPH / `{` found elsewhere)
* `PREFIX` / `BASE` (case-insensitive) → Turtle/TriG

### Phase 2 — N-Triples / N-Quads Grammar

If the prefix starts with `<` or `_:`, attempt a strict line-by-line parse
(`sniff_ntriples_or_nquads`). Each line must contain exactly 3 or 4 terms
followed by a dot. If any line fails, fall through to Phase 3.

* 3-term lines only → N-Triples (Medium)
* Any 4-term line → N-Quads (Medium)

### Phase 3 — Turtle / TriG Markers

Scan for syntax characters that are valid in Turtle/TriG but not in
N-Triples: `;` `,` `(` `)` `[` `]` `{` `}` and the bare keyword `a`
(rdf:type shorthand). Strings and IRIs are skipped to avoid false matches.

If markers are found, check for TriG-specific patterns (GRAPH keyword or
`{` outside strings) and return Turtle or TriG at Low confidence.

## Encoding Assumptions

All RDF syntax delimiters scanned by this module are ASCII bytes (0x00–0x7F).
In valid UTF-8, bytes in this range never appear as continuation bytes
(0x80–0xBF), so byte-level scanning for these markers is safe without full
UTF-8 decoding. Case-insensitive comparisons (e.g. for the GRAPH keyword and
file extensions) use `una::cases::to_lowercase_utf8()` for correctness.

## Confidence Levels

| Level   | Meaning                                                   |
|---------|-----------------------------------------------------------|
| None    | No guess could be made                                    |
| Low     | Weak heuristic (ambiguous extension, syntax markers only) |
| Medium  | Good signal from content sniffing                         |
| High    | Unambiguous extension or strong content match             |
| Certain | Extension and content agree                               |

## Format Precedence

When extension and content disagree:

* **High-confidence extension** wins over content.
* **Low-confidence extension** (`.owl`, `.xml`) defers to content if content
  produces a known result.
* **No extension** relies entirely on content.
