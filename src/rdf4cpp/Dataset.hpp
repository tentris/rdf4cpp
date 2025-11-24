#ifndef RDF4CPP_DATASET_HPP
#define RDF4CPP_DATASET_HPP

#include <rdf4cpp/Graph.hpp>
#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/query/QuadPattern.hpp>
#include <rdf4cpp/writer/BufWriter.hpp>
#include <rdf4cpp/util/Anonymizer.hpp>

#include <dice/sparse-map/sparse_map.hpp>

#include <memory>
#include <utility>

namespace rdf4cpp {


struct Dataset {
    using value_type = Quad;
    using sentinel = std::default_sentinel_t;
    using reference = Quad const &;
    using const_reference = reference;
    using pointer = Quad const *;
    using const_pointer = pointer;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

private:
    using storage_type = dice::sparse_map::sparse_map<storage::identifier::NodeBackendID, Graph>;

public:
    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = Quad;
        using difference_type = ptrdiff_t;
        using pointer = value_type const *;
        using reference = value_type const &;

    private:
        Dataset const *parent_;
        typename storage_type::const_iterator giter_;
        typename storage_type::const_iterator gend_;
        typename Graph::iterator iter_;

        Quad cur_;

    public:
        iterator(Dataset const *parent,
                 typename storage_type::const_iterator gbeg,
                 typename storage_type::const_iterator gend) noexcept;

        iterator &operator++() noexcept;
        void operator++(int) noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        friend bool operator==(iterator const &self, sentinel) noexcept;
        friend bool operator==(sentinel, iterator const &self) noexcept;
    };

    using const_iterator = iterator;

    struct solution_iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type = query::Solution;
        using difference_type = ptrdiff_t;
        using pointer = value_type const *;
        using reference = value_type const &;

    private:
        Dataset const *parent_;
        query::QuadPattern pat_;

        typename storage_type::const_iterator giter_;
        typename storage_type::const_iterator gend_;

        Graph::solution_iterator iter_;
        value_type cur_;

        void advance_until_result();
        void fill_solution() noexcept;

    public:
        solution_iterator(Dataset const *parent,
                          query::QuadPattern const &pat,
                          typename storage_type::const_iterator beg,
                          typename storage_type::const_iterator end) noexcept;

        solution_iterator &operator++() noexcept;
        void operator++(int) noexcept;
        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        friend bool operator==(solution_iterator const &self, sentinel) noexcept;
        friend bool operator==(sentinel, solution_iterator const &self) noexcept;
    };

    struct solution_sequence {
        using value_type = query::Solution;
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
    storage_type graphs_;

    static storage::identifier::NodeBackendID to_node_id(Node node) noexcept;
    Node to_node(storage::identifier::NodeBackendID id) const noexcept;

public:
    explicit Dataset(storage::DynNodeStoragePtr node_storage = storage::default_node_storage);

    void add(Quad const &quad);

    [[nodiscard]] bool contains(Quad const &quad) const noexcept;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t size(IRI const &graph_name) const noexcept;

    Graph *find_graph(Node const &graph);
    Graph *find_graph();

    Graph const *find_graph(Node const &graph) const;
    Graph const *find_graph() const;

    Graph &graph(Node const &graph);
    Graph &graph();

    [[nodiscard]] iterator begin() const noexcept;
    [[nodiscard]] sentinel end() const noexcept;

    [[nodiscard]] solution_sequence match(query::QuadPattern const &quad_pattern) const noexcept;

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
                add(*quad);
            } else {
                std::invoke(errf, quad.error());
            }
        }
    }

    /**
     * Serialize this dataset as <a href="https://www.w3.org/TR/n-quads/">N-Quads</a>.
     *
     * @param writer writer parts
     * @return true if serialization was successful, false if a call to flush was not able to make room
     */
    bool serialize(writer::BufWriterParts writer) const noexcept;

    /**
     * Serialize this dataset as <a href="https://www.w3.org/TR/rdf12-trig/">TriG</a>.
     * This function does not call `begin` or `flush` on the given state,
     * it just serialized the contents of this Dataset using it.
     *
     * @param state serialization state
     * @param writer writer parts
     * @return true if serialization was successful, false if a call to W::flush was not able to make room
     */
    bool serialize_trig(writer::SerializationState &state, writer::BufWriterParts writer) const noexcept;

    /**
     * Serialize this dataset as <a href="https://www.w3.org/TR/rdf12-trig/">TriG</a>.
     * This function will internally create a SerializationState and call `begin` and `flush`
     * on it when appropriate.
     *
     * @param writer writer parts
     * @return true if serialization was successful, false if a call to W::flush was not able to make room
     */
    bool serialize_trig(writer::BufWriterParts writer) const noexcept;

    friend std::ostream &operator<<(std::ostream &os, Dataset const &self);


    /**
     * Anonymize the dataset by removing all information except for the dataset structure itself.
     *
     * See `rdf4cpp::util::Anonymizer` for details
     */
    [[nodiscard]] Dataset anonymize(util::Anonymizer &anonymizer) const;

    // TODO: support union (+) and difference (-)
    // TODO: add empty
};


}  // namespace rdf4cpp
#endif  //RDF4CPP_DATASET_HPP
