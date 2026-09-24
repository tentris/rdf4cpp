#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <boost/multiprecision/cpp_int.hpp>

int main() {
    __int128 k = 100;
    boost::multiprecision::checked_int128_t v = 1000;

    ankerl::nanobench::Bench{}.relative(true)
            .run("builtin", [&k]() {
                for (int i=0;i<100;++i) {
                    __builtin_mul_overflow(k, k, &k);
                }
            })
            .run("boost", [&v]() {
                for (int i=0;i<100;++i) {
                    try {
                        v = v * v;
                    }
                    catch (...) {}
                }
            });

    std::cout << static_cast<int>(k) << v;
}
