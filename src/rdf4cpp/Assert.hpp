#ifndef RDF4CPP_ASSERT_HPP
#define RDF4CPP_ASSERT_HPP

#include <iostream>
#include <format>

#ifdef NDEBUG

#define RDF4CPP_ASSERT(expr)                                                                        \
	if (!(expr)) [[unlikely]] {                                                                     \
		std::cerr << std::format("Assertion failed: {} ({}:{})", #expr, __FILE__, __LINE__);        \
		std::abort();                                                                               \
	}

#define RDF4CPP_UNREACHABLE                                                                         \
	std::cerr << std::format("Unreachable statement reached ({}:{})", __FILE__, __LINE__);          \
    std::abort();

#define RDF4CPP_DEBUG_UNREACHABLE(...) return __VA_ARGS__

#else

#define RDF4CPP_ASSERT(expr) assert(expr)
#define RDF4CPP_UNREACHABLE assert(false)
#define RDF4CPP_DEBUG_UNREACHABLE(...) assert(false)

#endif
#define RDF4CPP_DEBUG_ASSERT(expr) assert(expr)

#endif  //RDF4CPP_ASSERT_HPP
