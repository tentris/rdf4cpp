#include "CharMatcher.hpp"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "rdf4cpp/util/CharMatcher.cpp"
#include <hwy/foreach_target.h>

#include <cassert>
#include <hwy/highway.h>
#include <stdexcept>
#include <rdf4cpp/Assert.hpp>


HWY_BEFORE_NAMESPACE();  // at file scope
namespace rdf4cpp::util::char_matcher_detail::HWY_NAMESPACE {
    // Calls `func(d, v)` for each input vector; out of bound lanes with index i >=
    // `count` are instead taken from `no[i % Lanes(d)]`.
    // if func returns false, exits early
    // from hwy/contrib/algo/transform-inl.h, added early return
    template<typename D, typename Func, typename T = hwy::HWY_NAMESPACE::TFromD<D>>
    HWY_INLINE void Foreach(D d, T const *HWY_RESTRICT in, size_t const count, hwy::HWY_NAMESPACE::Vec<D> const no,
                            Func const &func) {
        size_t const N = Lanes(d);

        size_t idx = 0;
        if (count >= N) {
            for (; idx <= count - N; idx += N) {
                hwy::HWY_NAMESPACE::Vec<D> const v = LoadU(d, in + idx);
                if (!func(d, v)) {
                    return;
                }
            }
        }

        // `count` was a multiple of the vector length `N`: already done.
        if (HWY_UNLIKELY(idx == count))
            return;

        size_t const remaining = count - idx;
        HWY_DASSERT(0 != remaining && remaining < N);
        hwy::HWY_NAMESPACE::Vec<D> const v = LoadNOr(no, d, in + idx, remaining);
        func(d, v);
    }

    template<size_t rn, size_t sn>
    requires(rn > 0)
    HWY_INLINE std::optional<bool> try_match_simd_impl(std::string_view data, std::array<CharRange, rn> const &ranges, datatypes::registry::util::ConstexprString<sn> const &single) {
        using namespace hwy::HWY_NAMESPACE;

        bool found_unicode = false;
        bool r = true;

        using D = ScalableTag<int8_t>;  //NOLINT tag type, selects the used vector type
        using V = Vec<D>;               //NOLINT vector type
        D const d;

        // elements from this vector are used, if data is not a multiple of Lanes(d)
        // should not influence final result
        // => comparisons need to evaluate as true for the logic to work
        V const zero = Set(d, static_cast<int8_t>(ranges[0].first));

#if !(HWY_TARGET & (HWY_ALL_NEON | HWY_ALL_SVE)) && !HWY_HAVE_SCALABLE
        // highway doc: on x86 < and > are 1 instruction for signed ints (3 for unsigned)
        // and <= and >= are 2 instructions regardless of signed/unsigned
        // Set is 2 instructions, while potential load/store is 1
        //
        // storing a vector directly is possible on x86
        //
        // => load vector once and store it inside an array

        using single_storage = V;
#define GET_SINGLE(x) x
#define LOAD_SINGLE(x) Set(d, x)
#else
        // highway doc: on arm neon <, >, <= and >= are 1 instruction for any int
        // Set is 1 instruction, while potential load/store is 1
        //
        // storing a vector directly is not possible on arm
        //
        // => store raw chars inside an array, load it when needed

        using single_storage = int8_t;
#define GET_SINGLE(x) Set(d, x)
#define LOAD_SINGLE(x) x
#endif

        // set up ranges
        std::array<std::pair<single_storage, single_storage>, rn> range_vectors;
        for (size_t i = 0; i < rn; ++i) {
            RDF4CPP_ASSERT(ranges[i].first != '\0');
            RDF4CPP_ASSERT(ranges[i].first < ranges[i].last);
            range_vectors[i].first = LOAD_SINGLE(static_cast<int8_t>(ranges[i].first - 1));
            range_vectors[i].second = LOAD_SINGLE(static_cast<int8_t>(ranges[i].last + 1));
        }

        single_storage const unicode_bit_index = LOAD_SINGLE(7);

        // set up single compares
        std::array<single_storage, sn - 1> single_vectors{};
        auto view = static_cast<std::string_view>(single);
        for (size_t i = 0; i < view.size(); ++i) {
            single_vectors[i] = LOAD_SINGLE(static_cast<int8_t>(view[i]));
        }

        Foreach(d, reinterpret_cast<int8_t const *>(data.data()), data.size(), zero, [&](auto d, auto in_vec) HWY_ATTR {
            // if bit 7 is set, the char belongs to a unicode sequence
            // => std::nullopt
            if (!AllTrue(d, Lt(HighestSetBitIndex(in_vec), GET_SINGLE(unicode_bit_index)))) {
                found_unicode = true;
                return false;
            }

            // check if target is in one of the ranges
            auto m = And(Gt(in_vec, GET_SINGLE(range_vectors[0].first)), Lt(in_vec, GET_SINGLE(range_vectors[0].second)));
            for (size_t i = 1; i < rn; ++i) {
                m = Or(m, And(Gt(in_vec, GET_SINGLE(range_vectors[i].first)), Lt(in_vec, GET_SINGLE(range_vectors[i].second))));
            }

            // check the single compares
            for (size_t i = 0; i < sn - 1; ++i) {
                m = Or(m, Eq(in_vec, GET_SINGLE(single_vectors[i])));
            }

            r = r && AllTrue(d, m);
            return r;  // possible early return
        });

#undef GET_SINGLE
#undef LOAD_SINGLE

        if (found_unicode)
            return std::nullopt;
        return r;
    }
    std::optional<bool> try_match_simd_impl_3_1(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<1> const &single) {
        return try_match_simd_impl(data, ranges, single);
    }
    std::optional<bool> try_match_simd_impl_3_4(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<4> const &single) {
        return try_match_simd_impl(data, ranges, single);
    }
    std::optional<bool> try_match_simd_impl_3_18(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<18> const &single) {
        return try_match_simd_impl(data, ranges, single);
    }
    std::optional<bool> try_match_simd_impl_3_20(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<20> const &single) {
        return try_match_simd_impl(data, ranges, single);
    }
    std::optional<bool> try_match_simd_impl_3_21(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<21> const &single) {
        return try_match_simd_impl(data, ranges, single);
    }
    std::optional<bool> try_match_simd_impl_1_1(std::string_view data, std::array<CharRange, 1> const &ranges, datatypes::registry::util::ConstexprString<1> const &single) {
        return try_match_simd_impl(data, ranges, single);
    }

    template<size_t n>
    HWY_INLINE bool contains_any_impl(std::string_view data, datatypes::registry::util::ConstexprString<n> const &match) {
        using namespace hwy::HWY_NAMESPACE;
        bool r = false;

        using D = ScalableTag<int8_t>;  //NOLINT  tag type, selects the used vector type
        using V = Vec<D>;               //NOLINT vector type
        D const d;

#if !(HWY_TARGET & (HWY_ALL_NEON | HWY_ALL_SVE)) && !HWY_HAVE_SCALABLE
        // highway doc: on x86 < and > are 1 instruction for signed ints (3 for unsigned)
        // and <= and >= are 2 instructions regardless of signed/unsigned
        // Set is 2 instructions, while potential load/store is 1
        //
        // storing a vector directly is possible on x86
        //
        // => load vector once and store it inside an array

        using single_storage = V;
#define GET_SINGLE(x) x
#define LOAD_SINGLE(x) Set(d, x)
#else
        // highway doc: on arm neon <, >, <= and >= are 1 instruction for any int
        // Set is 1 instruction, while potential load/store is 1
        //
        // storing a vector directly is not possible on arm
        //
        // => store raw chars inside an array, load it when needed

        using single_storage = int8_t;
#define GET_SINGLE(x) Set(d, x)
#define LOAD_SINGLE(x) x
#endif

        // elements from this vector are used, if data is not a multiple of Lanes(d)
        // should not influence final result
        // => comparisons need to evaluate as false for the logic to work
        V const zero = Set(d, static_cast<int8_t>(0));

        // load comparison vectors
        std::array<single_storage, n-1> match_vectors;
        auto view = static_cast<std::string_view>(match);
        for (size_t i = 0; i < n-1; ++i) {
            RDF4CPP_ASSERT(view[i] != '\0');
            match_vectors[i] = LOAD_SINGLE(static_cast<int8_t>(view[i]));
        }

        Foreach(d, reinterpret_cast<int8_t const *>(data.data()), data.size(), zero, [&](auto d, V in_vec) HWY_ATTR {
            // compare
            auto m = Eq(in_vec, GET_SINGLE(match_vectors[0]));
            for (size_t i = 1; i < n-1; ++i) {
                m = Or(m, Eq(in_vec, GET_SINGLE(match_vectors[i])));
            }

            r = r || !AllFalse(d, m);
            return !r;  // potential early return
        });

#undef GET_SINGLE
#undef LOAD_SINGLE

        return r;
    }

    bool contains_any_impl_5(std::string_view data, datatypes::registry::util::ConstexprString<5> const &match) {
        return contains_any_impl(data, match);
    }
    // NOLINTNEXTLINE(google-readability-namespace-comments)
}  // namespace rdf4cpp::util::char_matcher_detail::HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace rdf4cpp::util::char_matcher_detail {
    HWY_EXPORT(try_match_simd_impl_3_1);
    HWY_EXPORT(try_match_simd_impl_3_4);
    HWY_EXPORT(try_match_simd_impl_3_18);
    HWY_EXPORT(try_match_simd_impl_3_20);
    HWY_EXPORT(try_match_simd_impl_3_21);
    HWY_EXPORT(try_match_simd_impl_1_1);
    HWY_EXPORT(contains_any_impl_5);

    template<>
    std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<1> const &single) {
        return HWY_DYNAMIC_DISPATCH(try_match_simd_impl_3_1)(data, ranges, single);
    }
    template<>
    std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<4> const &single) {
        return HWY_DYNAMIC_DISPATCH(try_match_simd_impl_3_4)(data, ranges, single);
    }
    template<>
    std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<18> const &single) {
        return HWY_DYNAMIC_DISPATCH(try_match_simd_impl_3_18)(data, ranges, single);
    }
    template<>
    std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<20> const &single) {
        return HWY_DYNAMIC_DISPATCH(try_match_simd_impl_3_20)(data, ranges, single);
    }
    template<>
    std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 3> const &ranges, datatypes::registry::util::ConstexprString<21> const &single) {
        return HWY_DYNAMIC_DISPATCH(try_match_simd_impl_3_21)(data, ranges, single);
    }
    template<>
    std::optional<bool> try_match_simd(std::string_view data, std::array<CharRange, 1> const &ranges, datatypes::registry::util::ConstexprString<1> const &single) {
        return HWY_DYNAMIC_DISPATCH(try_match_simd_impl_1_1)(data, ranges, single);
    }

    template<>
    bool contains_any(std::string_view data, datatypes::registry::util::ConstexprString<5> const &match) {
        return HWY_DYNAMIC_DISPATCH(contains_any_impl_5)(data, match);
    }
}  // namespace rdf4cpp::util::char_matcher_detail
#endif
