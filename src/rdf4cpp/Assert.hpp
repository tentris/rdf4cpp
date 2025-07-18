#ifndef RDF4CPP_ASSERT_HPP
#define RDF4CPP_ASSERT_HPP

#include <iostream>

#ifdef NDEBUG

#define RDF4CPP_TOSTRING_2(x) #x
#define RDF4CPP_TOSTRING(x) RDF4CPP_TOSTRING_2(x)
#define RDF4CPP_ASSERT(expr)                                                                        \
	if (!(expr)) [[unlikely]] {                                                                     \
		std::cerr << "Assertion failed: " #expr " (" __FILE__ ":" RDF4CPP_TOSTRING(__LINE__) ")";   \
		std::abort();                                                                               \
	}

#define RDF4CPP_UNREACHABLE                                                                         \
	std::cerr << "Unreachable statement reached (" __FILE__ ":" RDF4CPP_TOSTRING(__LINE__) ")";     \
    std::abort();                                                                                   \
    __builtin_unreachable();

#define RDF4CPP_DEBUG_UNREACHABLE(...) return __VA_ARGS__

#else

#define RDF4CPP_ASSERT(expr) assert(expr)
#define RDF4CPP_UNREACHABLE assert(false)
#define RDF4CPP_DEBUG_UNREACHABLE(...) assert(false)

#endif
#define RDF4CPP_DEBUG_ASSERT(expr) assert(expr)

#endif  //RDF4CPP_ASSERT_HPP
