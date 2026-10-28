# defaults used when no compiler is given, neither via -DCMAKE_<LANG>_COMPILER nor via the CC/CXX environment variables
if (NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED ENV{CC})
  set(CMAKE_C_COMPILER gcc-14)
endif ()

if (NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
  set(CMAKE_CXX_COMPILER g++-14)
endif ()
