#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <format>
#include <string>
#include <vector>

#include <rdf4cpp.hpp>
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorage.hpp>
#include <rdf4cpp/storage/reference_node_storage/UnsyncReferenceNodeStorage.hpp>

using namespace rdf4cpp;

static std::vector<std::string> make_iris(size_t n) {
    std::vector<std::string> out;
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        out.push_back(std::format("http://data.semanticweb.org/conference/eswc/2009/paper/research/{}/author-{}", i, i * 7));
    }
    return out;
}

static void deserialize(char const *path, Dataset &ds, storage::DynNodeStoragePtr node_storage) {
    FILE *in_file = parser::fopen_fastseq(path, "r");
    setbuf(in_file, nullptr);
    parser::IStreamQuadIterator::state_type state{.node_storage = node_storage};
    parser::IStreamQuadIterator qit{in_file,
                                    reinterpret_cast<parser::ReadFunc>(&fread),
                                    reinterpret_cast<parser::ErrorFunc>(&ferror),
                                    reinterpret_cast<parser::EOFFunc>(&feof),
                                    parser::ParsingFlags::none(),
                                    &state};
    for (; qit != std::default_sentinel; ++qit) {
        if (qit->has_value()) {
            ds.add(**qit);
        }
    }
    fclose(in_file);
}

template<typename NS>
static void bench_storage(char const *label) {
    constexpr size_t n = 100'000;
    auto const iris = make_iris(n);

    // hit path: every IRI already exists in the node storage
    {
        NS ns{};
        for (auto const &s : iris) {
            ankerl::nanobench::doNotOptimizeAway(IRI{s, ns});
        }
        size_t i = 0;
        ankerl::nanobench::Bench{}
                .minEpochIterations(200'000)
                .run(std::format("{} IRI ctor, hit", label), [&] {
                    ankerl::nanobench::doNotOptimizeAway(IRI{iris[i++ % n], ns});
                });
    }

    // miss path: fresh storage, every IRI is new
    ankerl::nanobench::Bench{}
            .batch(n)
            .unit("iri")
            .run(std::format("{} IRI ctor, miss", label), [&] {
                NS ns{};
                for (auto const &s : iris) {
                    ankerl::nanobench::doNotOptimizeAway(IRI{s, ns});
                }
            });
}

int main(int argc, char **argv) {
    bench_storage<storage::reference_node_storage::UnsyncReferenceNodeStorage>("unsync");
    bench_storage<storage::reference_node_storage::SyncReferenceNodeStorage>("sync");

    if (argc > 1) {
        ankerl::nanobench::Bench{}
                .unit("SWDF")
                .epochs(5)
                .run("deserialization", [&] {
                    auto ns = storage::reference_node_storage::UnsyncReferenceNodeStorage{};
                    Dataset ds{ns};
                    deserialize(argv[1], ds, ns);
                });
    }
}
