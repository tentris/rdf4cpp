#ifndef RDF4CPP_GRAPH_HPP
#define RDF4CPP_GRAPH_HPP

#include <rdf4cpp/Statement.hpp>
#include <rdf4cpp/query/TriplePattern.hpp>
#include <rdf4cpp/query/Solution.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>
#include <rdf4cpp/writer/SerializationState.hpp>
#include <rdf4cpp/parser/RDFFileParser.hpp>

#include <dice/sparse-map/sparse_set.hpp>


namespace rdf4cpp {

struct Graph {
    using value_type = Statement;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = Statement const &;
    using const_reference = reference;
    using pointer = Statement const *;
    using const_pointer = pointer;

private:
    using triple = std::array<storage::identifier::NodeBackendID, 3>;

    struct triple_hash {
        size_t operator()(triple const &trip) const noexcept {
            return dice::hash::dice_hash_templates<dice::hash::Policies::wyhash>::dice_hash(trip);
        }
    };

    using triple_storage_type = dice::sparse_map::sparse_set<triple, triple_hash>;

public:
    using sentinel = std::default_sentinel_t;

    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = Statement;
        using difference_type = ptrdiff_t;
        using pointer = value_type const *;
        using reference = value_type const &;

    private:
        Graph const *parent_;
        typename triple_storage_type::const_iterator iter_{};
        typename triple_storage_type::const_iterator end_{};

        Statement cur_;

        Statement to_statement(triple const &t) const noexcept;

    public:
        iterator() noexcept = default;
        iterator(Graph const *parent, typename triple_storage_type::const_iterator beg, typename triple_storage_type::const_iterator end) noexcept;

        iterator &operator++() noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        bool operator==(sentinel) const noexcept;
        bool operator!=(sentinel) const noexcept;
    };

    using const_iterator = iterator;

    struct solution_iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = query::Solution;
        using difference_type = ptrdiff_t;
        using pointer = value_type const *;
        using reference = value_type const &;

    private:
        typename Graph::iterator iter_;
        query::TriplePattern pat_;
        value_type cur_;

        bool check_solution() noexcept;
        void forward_to_solution() noexcept;

    public:
        solution_iterator() noexcept = default;
        solution_iterator(typename Graph::iterator beg,
                          query::TriplePattern const &pat) noexcept;

        solution_iterator &operator++() noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        bool operator==(sentinel) const noexcept;
        bool operator!=(sentinel) const noexcept;
    };

    struct solution_sequence {
        using value_type = Statement;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using reference = value_type const &;
        using const_reference = reference;
        using pointer = value_type const *;
        using const_pointer = pointer;
        using iterator = solution_iterator;
        using const_iterator = solution_iterator;
        using sentinel = std::default_sentinel_t;

    private:
        iterator beg_;

    public:
        explicit solution_sequence(iterator beg) noexcept : beg_{beg} {
        }

        [[nodiscard]] iterator begin() const noexcept {
            return beg_;
        }

        [[nodiscard]] static sentinel end() noexcept {
            return sentinel{};
        }
    };

private:
    storage::DynNodeStoragePtr node_storage_;
    triple_storage_type triples_;

    static storage::identifier::NodeBackendID to_node_id(Node node) noexcept;
    Node to_node(storage::identifier::NodeBackendID id) const noexcept;

public:
    explicit Graph(storage::DynNodeStoragePtr node_storage = storage::default_node_storage) noexcept;

    void add(Statement const &statement);

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool contains(Statement const &statement) const noexcept;

    [[nodiscard]] solution_sequence match(query::TriplePattern const &triple_pattern) const noexcept;

    [[nodiscard]] iterator begin() const noexcept;
    [[nodiscard]] sentinel end() const noexcept;

    template<typename ErrF = decltype([](parser::ParsingError) noexcept {})>
    void load_rdf_data(std::istream &rdf_file,
                       parser::ParsingFlags flags = parser::ParsingFlags::none(),
                       parser::ParsingState *state = nullptr,
                       ErrF &&errf = {}) requires std::invocable<decltype(errf), parser::ParsingError> {

        if (state != nullptr && state->node_storage != node_storage_) {
            throw std::invalid_argument{"NodeStorage of the parsing state must be the same as NodeStorage of the Dataset"};
        }

        parser::IStreamQuadIterator parser{rdf_file, flags, state};
        for (; parser != std::default_sentinel; ++parser) {
            auto const &quad = *parser;
            if (quad.has_value()) {
                add(Statement{quad->subject(), quad->predicate(), quad->object()});
            } else {
                std::invoke(errf, quad.error());
            }
        }
    }

    /**
     * Serialize this graph as <a href="https://www.w3.org/TR/n-triples/">N-Triples</a>.
     *
     * @param writer writer parts
     * @return true if serialization was successful, false if a call to W::flush was not able to make room
     */
    bool serialize(writer::BufWriterParts writer) const noexcept;

    /**
     * Serialize this graph as <a href="https://www.w3.org/TR/rdf12-turtle/">Turtle</a>
     * This function does not call `begin` or `flush` on the given state,
     * it just serialized the contents of this Graph using it.
     *
     * @param writer writer parts
     * @return true if serialization was successful, false if a call to W::flush was not able to make room
     */
    bool serialize_turtle(writer::SerializationState &state, writer::BufWriterParts writer) const noexcept;

    /**
     * Serialize this graph as <a href="https://www.w3.org/TR/rdf12-turtle/">Turtle</a>
     * This function will internally create a SerializationState and call `begin` and `flush`
     * on it when appropriate.
     *
     * @param writer writer parts
     * @return true if serialization was successful, false if a call to W::flush was not able to make room
     */
    bool serialize_turtle(writer::BufWriterParts writer) const noexcept;

    bool serialize_json_ld(writer::SerializationState &state, writer::BufWriterParts writer) const noexcept;

    /**
     * Serialize this graph as <a href="https://www.w3.org/TR/n-triples/">N-Triples</a>.
     */
    friend std::ostream &operator<<(std::ostream &os, Graph const &graph);

    // TODO: support union (+) and difference (-); open question: which graph name should be assigned?
    // TODO: add empty
};
}  // namespace rdf4cpp

#endif  //RDF4CPP_GRAPH_HPP
